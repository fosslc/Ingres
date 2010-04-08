/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
**
******************************************************************************/

#include <compat.h>
#include <ut.h>
#include <wpspost.h>
#include <erwsf.h>
#include <usrsession.h>
#include <postpars.h>
#include <wcs.h>
#include <htmlmeth.h>
#include <wpsreport.h>
#include <wpsapp.h>
#include <wsmvar.h>

/*
**
**  Name: wpspost.c - Posted execution. None macro command
**
**  Description:
**
**  History:
**	5-Feb-98 (marol01)
**	    created
**      08-Sep-1998 (fanra01)
**          Corrected case for usrsession to match piccolo.
**          Separated PARSER_IN initialisation from declaration for unix.
**      27-Nov-1998 (fanra01)
**          Add the setting of user and password from HTML variables during
**          the execution of a non macro query.  The user id is the alias name
**          of a dbuser in the repository.
**      15-Dec-1998 (fanra01)
**          Allow execution of db procedure to use existing session
**          information.
**      05-Jan-1999 (fanra01)
**          Ensure that the returned string is terminated when appending to
**          the block.  Prevents string concatinations.
**      18-Jan-1999 (fanra01)
**          Cleanup compiler warnings on unix.
**      29-Mar-1999 (fanra01)
**          Add release of generated files for reports and applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Additional parameters for DBPrepare for handling multi row
**          result sets.
**      20-Jun-2001 (fanra01)
**          Sir 103096
**          Add the setting of query output format type from an HTML variable
**          ii_rel_type.
**      20-Sep-2001 (fanra01)
**          Bug 105835
**          Terminate a transaction after the query has been processed.
**          This has the affect of releasing the database connection back
**          to the connection pool.
**      19-Oct-2001 (fanra01)
**          Bug 106100
**          An invalidated transaction pointer causes an exception when
**          attempting to destroy the transaction.  Additional checks.
**      25-Feb-2002 (hweho01)
**          Fix the syntax in DDFStatusInfo() call. 
**      22-Jul-2002 (fanra01)
**          Bug 108329
**          Modify handling of error codes when specifically OK or fail.
**      20-Aug-2003 (fanra01)
**          Bug 110747
**          Additional output information if parse error occurs.
**          Add test for null parameter to report invocation.
**	10-dec-2004 (abbjo03)
**	    Add missing include of compat.h.
**/


/*
** Name: WPSFormatResult() - Format the result of the posted execution
**
** Description:
**
** Inputs:
**	bool		: usrSuccess
**	bool		: usrError
**	GSTATUS		: result
**
** Outputs:
**	WPS_PBUFFER	: buffer,
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      05-Jan-1999 (fanra01)
**          Ensure that the returned string is terminated.
**      22-Jul-2002 (fanra01)
**          Ensure that procexec return values are processed according to
**          whether a handling URL is specified.
*/

GSTATUS
WPSFormatResult (
    bool        usrSuccess,
    bool        usrError,
    WPS_PBUFFER    buffer,
    GSTATUS        result)
{
    GSTATUS err = GSTAT_OK;
    char*   pszUsrMsg = NULL;

    if ((result != GSTAT_OK) &&
        ((result->number == E_WS0031_PROCEXEC_OK && usrSuccess == FALSE) ||
        (result->number == E_WS0032_PROCEXEC_FAIL && usrError == FALSE)) ||
        (result->number != E_WS0032_PROCEXEC_FAIL))
    {
        CL_ERR_DESC clError;
        i4  nMsgLen;
        char szMsg[ERROR_BLOCK];

        ERslookup (
            result->number,
            NULL,
            0,
            NULL,
            szMsg,
            ERROR_BLOCK,
            -1,
            &nMsgLen,
            &clError,
            0,
            NULL);
        szMsg[nMsgLen] = EOS;

        err = WPSBlockAppend(buffer, szMsg, nMsgLen+1);
        if (err == GSTAT_OK)
        {
            err = WPSBlockAppend(buffer, TAG_PARAGRAPH_BREAK,
                STlength(TAG_PARAGRAPH_BREAK));
        }
        if (err == GSTAT_OK)
            err = WPSBlockAppend(buffer, result->info, STlength(result->info));
    }
    if ((result != GSTAT_OK) &&
        (result->number != E_WS0031_PROCEXEC_OK))
    {
        err = result;
    }
    return(err);
}

/*
** Name: WPSExecuteProcedure() - Execute a database procedure
**
** Description:
**
** Inputs:
**	bool		: usrSuccess
**	bool		: usrError
**	ACT_PSESSION: active session,
**	char*		: procedure name
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      06-Oct-98 (fanra01)
**          Fix compiler warning.
**          Save value of HVAR_PROCVAR_COUNT into its own variable as using
**          WSMGetVariable will change the value of temp and corrupt the
**          heap.
**      15-Dec-1998 (fanra01)
**          Do not get user or password if the session is setup.  Executing
**          procedure with session information.
**      16-Mar-2001 (fanra01)
**          Add rowset and setsize parameters for DBPrepare.
**      20-Sep-2001 (fanra01)
**          Add a call to free the transaction after the query has been
**          processed.
**      19-Oct-2001 (fanra01)
**          Add checks for transaction and session pointers before attempting
**          to destroy the transaction.
*/

GSTATUS
WPSExecuteProcedure (
	bool		usrSuccess,
	bool		usrError,
	ACT_PSESSION	act_session,
	WPS_PBUFFER	 buffer,
	char		*name)
{
    GSTATUS     status = GSTAT_OK;
    GSTATUS     err = GSTAT_OK;
    i4          i = 0;
    char*       pszType = NULL;
    char*       pszValue = NULL;
    char*       pszLength = NULL;
    char*       pszName = NULL;
    i4          itemLen = 0;
    char*       hvar = NULL;
    char*       temp = NULL;
    i4          cProcVar = 0;
    char*       stmt = NULL;
    u_i4       stmtLength = 0;
    u_i4       stmtMax = 0;
    PDB_VALUE   values = NULL;
    u_i4       length = 0;
    u_i4       counter = 0;
    bool        bad_var = FALSE;
    bool        bad_value = FALSE;
    CL_ERR_DESC clError;
    i4          nMsgLen;
    char        szMsg[ERROR_BLOCK];

    err = G_ME_REQ_MEM(act_session->mem_tag, temp, char, NUM_MAX_HVAR_LEN + NUM_MAX_PARM_STR + 1);
    if (err == GSTAT_OK)
	err = G_ME_REQ_MEM(act_session->mem_tag, stmt, char, WSF_READING_BLOCK);
    if (err == GSTAT_OK)
    {
        stmtMax = WSF_READING_BLOCK;
        STprintf(stmt, "execute procedure %s (", name);
        stmtMax = STlength(stmt);

        err = WSMGetVariable (act_session, HVAR_PROCVAR_COUNT,
            STlength(HVAR_PROCVAR_COUNT), &hvar,
            WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
        if (err == GSTAT_OK &&
            (hvar[0] == EOS || (CVan (hvar, &cProcVar) != OK) ||
                cProcVar < 0))
            err = DDFStatusAlloc(E_WS0029_BAD_PROCVAR_COUNT);

        if (err == GSTAT_OK && cProcVar > 0)
            err = G_ME_REQ_MEM(act_session->mem_tag, values, DB_VALUE, cProcVar);

        for (i = 1; err == GSTAT_OK && i <= cProcVar; i++)
        {
            STprintf(temp, "%s%d", HVAR_PROCVAR_TYPE, i);
            err = WSMGetVariable (act_session, temp, STlength(temp), &pszType,
                WSM_ACTIVE);
            if (err == GSTAT_OK && status == GSTAT_OK &&
                (pszType == NULL || pszType[0] == EOS))
            {
                bad_var = TRUE;
                status = DDFStatusAlloc(E_WS0019_BAD_PROCVAR_TYPE);
            }

    	    if (err == GSTAT_OK)
            {
                STprintf(temp, "%s%d", HVAR_PROCVAR_LEN, i);
                err = WSMGetVariable (act_session, temp, STlength(temp),
                    &pszLength, WSM_ACTIVE);
                if (err == GSTAT_OK && status == GSTAT_OK &&
                    (pszLength == NULL ||
                    (pszLength != NULL && (CVan (pszLength, &itemLen) != OK))))
                {
                    bad_value = TRUE;
                    status = DDFStatusAlloc(E_WS0020_BAD_PROCVAR_LENGTH);
                }
            }

    	    if (err == GSTAT_OK)
            {
                STprintf(temp, "%s%d", HVAR_PROCVAR_NAME, i);
                err = WSMGetVariable (act_session, temp, STlength(temp),
                    &pszName, WSM_ACTIVE);
                if (err == GSTAT_OK && status == GSTAT_OK &&
                    (pszName == NULL || pszName[0] == EOS))
                {
                    bad_var = TRUE;
                    status = DDFStatusAlloc(E_WS0021_BAD_PROCVAR_NAME);
                }
            }

    	    if (err == GSTAT_OK)
            {
                STprintf(temp, "%s%d", HVAR_PROCVAR_DATA, i);
                err = WSMGetVariable (act_session, temp, STlength(temp),
                    &pszValue, WSM_ACTIVE);
                if (err == GSTAT_OK && status == GSTAT_OK && pszValue == NULL)
                {
                    bad_value = TRUE;
                    status = DDFStatusAlloc(E_WS0022_BAD_PROCVAR_DATA);
                }
            }

            if (err == GSTAT_OK)
            {
                if (bad_var == FALSE)
                    length = STlength(pszName);

                if (bad_value == FALSE)
                    length += STlength(pszValue);

                length += (STlength(STR_ERROR_MARK) + 10);

                if (length > stmtMax)
                {
                    char *tmp = stmt;
                    err = G_ME_REQ_MEM(act_session->mem_tag, stmt, char, stmtMax + length);
                    if (err == GSTAT_OK)
                    {
                        MECOPY_VAR_MACRO(tmp, stmtMax, stmt);
                        stmtMax += length;
                        MEfree(tmp);
                    }
                    else
                        stmt = tmp;
                }

                if (err == GSTAT_OK)
                {
                    if (counter++ != 0)
                        STcat(stmt, ",");

                    if (bad_var != TRUE && bad_value != TRUE)
                    {
                        switch (pszType[0])
                        {
                            case 'f': /* f4, f8 */
                            case 'i': /* i2, i4, i1*/
                                STpolycat(4, stmt, pszName, " = ", pszValue,
                                    stmt);
                                break;
                            case 'b': /* byte */
                            case 'v': /* varchar */
                            case 'c': /* char, c */
                            case 't': /* text */
                            case 'l': /* long byte and long varchar */
                                STpolycat(5, stmt, pszName, " = '", pszValue,
                                    "'", stmt);
                                break;
                            default:
                                if (status == GSTAT_OK)
                                    status = DDFStatusAlloc(E_WS0019_BAD_PROCVAR_TYPE);
                                bad_var = TRUE;
                        }
                    }

                    if (bad_var == TRUE)
                        STcat(stmt, STR_ERROR_MARK);
                    else
                        if (bad_value == TRUE)
                            STpolycat(4, stmt, pszName, " = ", STR_ERROR_MARK,
                                stmt);
                }
    	    }
        }
        if (err == GSTAT_OK)
        {
            STcat (stmt, STR_CPAREN);
        }
    }

    if (err == GSTAT_OK)
	err = status;

    if (err == GSTAT_OK)
    {
	USR_PTRANS transaction = NULL;
	char*	dbname = NULL;
	char*	user = NULL;
	char*	password = NULL;

        if (err == GSTAT_OK)
            err = WSMGetVariable (
                      act_session,
                      HVAR_DATABASE,
                      STlength(HVAR_DATABASE),
                      &dbname,
                      WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

        if (act_session->user_session == NULL)
        {
            if (err == GSTAT_OK)
                err = WSMGetVariable (
                          act_session,
                          HVAR_USERID,
                          STlength(HVAR_USERID),
                          &user,
                          WSM_SYSTEM | WSM_ACTIVE | WSM_USER);

            if (err == GSTAT_OK)
                err = WSMGetVariable (
                          act_session,
                          HVAR_PASSWORD,
                          STlength(HVAR_PASSWORD),
                          &password,
                          WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
        }
	if (err == GSTAT_OK)
	{
	    u_i4 driver;
	    err = DriverLoad(OPEN_INGRES_DLL, &driver);
	    if (err == GSTAT_OK)
		err = WSMCreateTransaction(
					   act_session->user_session,
					   driver,
					   dbname,
					   NULL, /* transaction name */
					   user,
					   password,
					   &transaction);
	}
	if (err == GSTAT_OK)
	{
	    DB_QUERY query;
	    err = DBPrepare(transaction->session->driver)(&query, stmt,
                transaction->rowset, transaction->setsize );
	    if (err == GSTAT_OK)
	    {
		err = DBExecute(transaction->session->driver)(&(transaction->session->session), &query);
		if (err == GSTAT_OK)
		    err = DBClose(transaction->session->driver)(&query, NULL);
		CLEAR_STATUS(DBRelease(transaction->session->driver)(&query));
	    }
	    if (err == GSTAT_OK)
		err = WSMCommit(transaction, FALSE);
	    else
		CLEAR_STATUS(WSMRollback(transaction, FALSE));
            /*
            ** Add a call to remove the transaction after it's done.
            */
            if ((err == GSTAT_OK) && (transaction != NULL) &&
                (transaction->session != NULL))
            {
                err = WSMDestroyTransaction( transaction );
            }
	}
    }

    if (err == GSTAT_OK)
	err = DDFStatusAlloc(E_WS0031_PROCEXEC_OK);
    else
    {
	DDFStatusFree(TRC_INFORMATION, &err);
	err = DDFStatusAlloc(E_WS0032_PROCEXEC_FAIL);
    }

    ERslookup (
	       I_WS0030_PROC_INFO,
	       NULL,
	       0,
	       NULL,
	       szMsg,
	       sizeof(szMsg),
	       -1,
	       &nMsgLen,
	       &clError,
	       0,
	       NULL);
    szMsg[nMsgLen] = EOS;

    DDFStatusInfo(err, szMsg, name, stmt);
	err = WPSFormatResult(usrSuccess, usrError, buffer, err);
    MEfree(stmt);
    MEfree(temp);
    return(err);
}

/*
** Name: WPSExecuteQuery() - Execute a query
**
** Description:
**
** Inputs:
**	bool		: usrSuccess
**	bool		: usrError
**	ACT_PSESSION: active session,
**	char*		: query
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      08-Sep-1998 (fanra01)
**          Separated PARSER_IN initialisation from declaration for unix.
**      08-Oct-98 (fanra01)
**          Change initialisation of in.size to in.length as size not used
**          also bump length to include null terminator.
**          Reorganised to allocate memory for the out structure and to
**          initialise it.
**          Modified action during error output to enclose message within
**          html and body tags.
**      27-Nov-1998 (fanra01)
**          If user and password are passed as HTML variables use these
**          values.  The user must be an aliased dbuser in the repository.
**      20-Jun-2001 (fanra01)
**          Add initialisation of query output type from HTML variable and
**          use the xml version tag instead of the html document tags for xml
**          documents.
**      10-Jul-2003 (fanra01)
**          Add additional information to add query text to the error when
**          an error is returned from the parser.
*/
GSTATUS
WPSExecuteQuery (
    bool            usrSuccess,
    bool            usrError,
    ACT_PSESSION    act_session,
    WPS_PBUFFER     buffer,
    char*           query,
    i4              outtype)
{
    GSTATUS     err = GSTAT_OK;
    char        szConfig [CONF_STR_SIZE];
    char*       allowed = NULL;
    CL_ERR_DESC clError;
    i4          nMsgLen;
    char        szMsg[ERROR_BLOCK];

    STprintf (szConfig, CONF_ALLOW_DYN_SQL, PMhost());
    if (query != NULL && query[0] != EOS &&
        PMget( szConfig, &allowed ) == OK && allowed != NULL &&
        STbcompare(allowed, 0, ON, 0, TRUE) == 0)
    {
        PARSER_IN in;
        WPS_HTML_PPARSER_OUT out;

        MEfill (sizeof (PARSER_IN), 0, (PTR)&in);
        in.first_node = POST_FIRST_NODE;
        in.buffer = query;
        in.length = STlength(query) + 1;

        err = G_ME_REQ_MEM(0, out, WPS_HTML_PARSER_OUT, 1);
        if (err == GSTAT_OK) 
        {
            out->act_session = act_session;
            out->result = buffer;

            err    = WPSBlockInitialize(&out->buffer);
            if ((outtype != XML_ICE_TYPE_XML) &&
                (outtype != XML_ICE_TYPE_XML_PDATA))
            {
                if (err == GSTAT_OK)
                {
                    err = WPSBlockAppend(
                            buffer,
                            TAG_HTML,
                            STlength(TAG_HTML));
                    if (err == GSTAT_OK)
                        err = WPSBlockAppend(
                            buffer,
                            TAG_BODY,
                            STlength(TAG_BODY));
                }
            }
            else
            {
                err = WPSBlockAppend(
                        buffer,
                        STR_XML_VERSION,
                        STlength(STR_XML_VERSION));
            }
            if (err == GSTAT_OK)
            {
                err = ParseOpen(post, &in, (PPARSER_OUT) out);
                if (err == GSTAT_OK)
                {
                    char *dbuser = NULL;
                    char *dbpass = NULL;

                    err = WPSHtmlICE(&in, (PPARSER_OUT)out);
                    out->version20.query.type = outtype;
                    /*
                    ** Set user and password here from HTML variables
                    */
                    err = WSMGetVariable (act_session, HVAR_USERID,
                        STlength(HVAR_USERID), &dbuser,
                        WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
                    if (dbuser && *dbuser)
                        out->version20.query.user = STalloc(dbuser);

                    err = WSMGetVariable (act_session, HVAR_PASSWORD,
                        STlength(HVAR_PASSWORD), &dbpass,
                        WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
                    if (dbpass && *dbpass)
                        out->version20.query.password = STalloc(dbpass);

                    if (err == GSTAT_OK) 
                    {
                        err = WPSHtmlNewSqlQuery(&in, (PPARSER_OUT)out);
                        in.position = 0;
                        if (err == GSTAT_OK)
                            err = Parse(post, &in, (PPARSER_OUT) out);
                        if (err == GSTAT_OK)
                            err = ParseClose(post, &in, (PPARSER_OUT) out);
                        if (err != GSTAT_OK)
                        {
                            DDFStatusInfo( err, "Query = %s\n", query );
                        }
                        if (err == GSTAT_OK)
                            err = WPSMoveBlock(out->result, &out->buffer);
                        CLEAR_STATUS(WPSHtmlEnd(&in, (PPARSER_OUT)out));
                    }
                }
            }
            if (err == GSTAT_OK)
            {
                if (out->active_error == 0)
                {
                    /*    err = DDFStatusAlloc(E_WS0025_OK_STATEMENT); */
                }
                else
                {
                    DDFStatusFree(TRC_INFORMATION, &err);
                    err = DDFStatusAlloc(E_WS0024_BAD_STATEMENT);
                    ERslookup (
                        I_WS0033_STATEMENT_INFO,
                        NULL,
                        0,
                        NULL,
                        szMsg,
                        ERROR_BLOCK,
                        -1,
                        &nMsgLen,
                        &clError,
                        0,
                        NULL);
                    szMsg[nMsgLen] = EOS;

                    DDFStatusInfo(err, szMsg, query);
                }
                if (err != GSTAT_OK)
                {
                    err = WPSBlockAppend(
                            buffer,
                            TAG_HTML,
                            STlength(TAG_HTML));
                    if (err == GSTAT_OK)
                        err = WPSBlockAppend(
                            buffer,
                            TAG_BODY,
                            STlength(TAG_BODY));
                    err    = WPSFormatResult(usrSuccess, usrError, buffer, err);
                    err = WPSBlockAppend(
                                buffer,
                                TAG_END_BODY,
                                STlength(TAG_END_BODY));
                    if (err == GSTAT_OK)
                        err = WPSBlockAppend(
                                buffer,
                                TAG_END_HTML,
                                STlength(TAG_END_HTML));
                }
            }

            if ((outtype != XML_ICE_TYPE_XML) &&
                (outtype != XML_ICE_TYPE_XML_PDATA))
            {
                if (err == GSTAT_OK)
                {
                    err = WPSBlockAppend(
                                buffer,
                                TAG_END_BODY,
                                STlength(TAG_END_BODY));
                    if (err == GSTAT_OK)
                        err = WPSBlockAppend(
                                buffer,
                                TAG_END_HTML,
                                STlength(TAG_END_HTML));
                }
            }
            MEfree((PTR)out);
        }
    }
    else
    {
        err = DDFStatusAlloc(E_WS0044_ILLEGAL_STMT);
        DDFStatusInfo (err , query);
    }
return (err);
}

/*
** Name: WPSExecuteReport() - Execute a report
**
** Description:
**
** Inputs:
**	bool		: usrSuccess
**	bool		: usrError
**	ACT_PSESSION: active session,
**	char		: report name
**	char*		: location name
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      06-Oct-1998 (fanra01)
**          Check for a null location and pass an empty string.
**      29-Mar-1999 (fanra01)
**          Add release of generated files.  These files will be deleted when
**          their timeout expires.
*/
GSTATUS
WPSExecuteReport (
	bool		usrSuccess,
	bool		usrError,
	ACT_PSESSION act_session,
	WPS_PBUFFER buffer,
	char* name,
	char* location)
{
    GSTATUS		err = GSTAT_OK;
    char *		pszParams = NULL;
    WPS_PFILE	ploOut;
    WPS_PFILE	ploErr;
    char*		url = NULL;
    u_i4       ixParam = 0;
    char*       pszCmdFmt = NULL;
    char*       apszParam[MAX_RW_PARAM];
    char*		dbname = NULL;

    if (err == GSTAT_OK)
    {
	err = WPSGetReportParameters(act_session, &pszParams);
	if (err == GSTAT_OK)
	{
	    err = WPSGetOutputDirLoc (act_session, &ploOut, &ploErr);
	    if (err == GSTAT_OK)
	    {
		err = WSMGetVariable (act_session, HVAR_DATABASE, STlength(HVAR_DATABASE), &dbname, WSM_SYSTEM | WSM_ACTIVE | WSM_USER);
		if (err == GSTAT_OK)
		    err = WPSGetReportCommand(&pszCmdFmt, &ixParam, apszParam, act_session, dbname, name, location, pszParams, &(ploOut->file->loc));

		if (err == GSTAT_OK)
		{
		    STATUS      status;
		    CL_ERR_DESC errCode;
		        status = UTexe (UT_WAIT,
				    &(ploErr->file->loc),
				    (PTR)NULL, (PTR)NULL,
				    STR_ING_RW_UTEXE_NAME,
				    &errCode,
				    pszCmdFmt,
				    ixParam,
				    apszParam[0],
				    apszParam[1],
				    apszParam[2],
				    apszParam[3],
				    apszParam[4],
				    apszParam[5]);
		    if (status != OK)
		    {
                if (usrError == FALSE)
                {
                    /*
                    ** Add test of command argument for null
                    */
                    err = WPSReportError (
						  act_session,
						  buffer,
						  (pszCmdFmt) ? pszCmdFmt : "\0",
						  apszParam,
						  ploErr);
                }
		    }
		    else
		    {
			if (usrSuccess == FALSE)
			    err = WPSReportSuccess(
						   act_session,
						   buffer,
						   name,
                           ((location != NULL) ? location : ""),
						   ploOut);
		    }
		}
		MEfree (pszCmdFmt);
                CLEAR_STATUS( WPSRelease(&ploOut) );
                CLEAR_STATUS( WPSRelease(&ploErr) );
	    }
	}
    }
    return (err);
}

/*
** Name: WPSExecuteApp () - Execute an external application
**
** Description:
**
** Inputs:
**	bool		: usrSuccess
**	bool		: usrError
**	ACT_PSESSION: active session,
**	char		: application name
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      07-Oct-1998 (fanra01)
**          Change condition testing for allowing appliction excution.
**          Allocate stack storage for loOut and initialise.
**      29-Mar-1999 (fanra01)
**          Add release of generated file.  This file will be deleted when its
**          timeout expires.
*/
GSTATUS
WPSExecuteApp (
    bool            usrSuccess,
    bool            usrError,
    ACT_PSESSION    act_session,
    WPS_PBUFFER     buffer,
    char*           pszApp)
{
    GSTATUS     err = GSTAT_OK;
    i4          length = 0;
    char        *cmdline = NULL;
    char        *params = NULL;
    char        *url = NULL;
    char        *relpath = NULL;
    char        app_results [SI_MAX_TXT_REC + 1];
    char        *apploc = NULL;
    WCS_FILE    wcsfile;
    WPS_FILE    loOut;
    WPS_PFILE   ploOut;
    CL_ERR_DESC error_code;
    bool        parent = FALSE;
    char        szConfig [CONF_STR_SIZE];
    char*       allowed = NULL;
    char        appspec[MAX_LOC+1];

    loOut.file = &wcsfile;

    STprintf (szConfig, CONF_ALLOW_EXE_APP, PMhost());
    if (PMget( szConfig, &allowed ) == OK && allowed != NULL &&
        STbcompare(allowed, 0, ON, 0, TRUE)==0)
    {
        err = WPSAppParentDirReference (pszApp, &parent);
        if (err == GSTAT_OK && parent == FALSE)
        {
            err = WPSAppAdditionalParams (act_session, &params);
            if (err == GSTAT_OK)
            {
                /* Get the full path to the application together */
                STprintf (szConfig, CONF_BIN_DIR, PMhost());
                PMget( szConfig, &apploc );
                STcopy (apploc, appspec);
                LOfroms (PATH, appspec, &(loOut.file->loc));
                LOfstfile (pszApp, &(loOut.file->loc));
                LOtos (&(loOut.file->loc), &apploc);
                /*
                ** Concatenate the application name along with any
                ** parameters
                */
                err = G_ME_REQ_MEM(act_session->mem_tag, cmdline, char, STlength (apploc) + STlength (STR_SPACE) + STlength (params) + 1);
                if (err == GSTAT_OK)
                {
                    STpolycat (3,apploc, STR_SPACE, params, cmdline);
                    /* Get an output file */
                    err = WPSGetOutputDirLoc (act_session, &ploOut, NULL);
                    if (err == GSTAT_OK)
                    {
                        /* Execute the command */
                        PCcmdline (NULL, cmdline, PC_WAIT,
                            &(ploOut->file->loc), &error_code);

                        err = WCSOpen (ploOut->file, "r", SI_BIN, 0);
                        if (err == GSTAT_OK)
                        {
                            bool hasTag = FALSE;

                            err = WCSRead (ploOut->file, SI_MAX_TXT_REC,
                                &length, app_results);
                            if (err == GSTAT_OK && length > 0)
                            {
                                app_results[length] = EOS;
                                err = HasTags(app_results, &hasTag);

                                if (err == GSTAT_OK && hasTag == FALSE)
                                err = WPSAppBegin(act_session, buffer, pszApp);

                                if (err == GSTAT_OK)
                                err = WPSBlockAppend(
                                    buffer,
                                    app_results,
                                    length);

                                err = WCSRead (ploOut->file, SI_MAX_TXT_REC,
                                    &length, app_results);
                                while ( err == GSTAT_OK && length > 0)
                                {
                                    err = WPSBlockAppend(
                                    buffer,
                                    app_results,
                                    length);

                                    if (err == GSTAT_OK)
                                        err = WCSRead (ploOut->file,
                                            SI_MAX_TXT_REC, &length,
                                            app_results);
                                }

                                if (err == GSTAT_OK && hasTag == FALSE)
                                err = WPSAppEnd(act_session, buffer);
                            }
                            CLEAR_STATUS( WCSClose(ploOut->file) );
                        }
                        CLEAR_STATUS( WPSRelease(&ploOut) );
                    }
                    MEfree(cmdline);
                }
            }
        }
    }
    else
        err = DDFStatusAlloc(E_WS0034_FORBIDDEN_ACTION);
    return (err);
}
