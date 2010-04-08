/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : sqlcopy.sc
** Project  : Data Loader
** Author   : UK Sotheavut (uk$so01) 
** Purpose  : SQL Session & SQL Copy
**

** History:
**
** 02-Jan-2004 (uk$so01)
**    created
** 21-Jun-2004 (uk$so01)
**    SIR #111688, additional change to display ingres
**    error message when sqlca.sqlcode = -33000 and the 
**    GetRow handler has never been called.
** 29-Jun-2004 (uk$so01)
**    SIR #111688, additional change to support -u<username> option
**    at the command line.
** 12-May-2006 (thaju02)
**    In INGRESII_llConnectSession(), determine if database is NFC/NFD
**    and intialize collation table for adf_cb.
** 29-jan-2008 (huazh01)
**    replace all declarations of 'long' type variable into 'i4'.
**    Bug 119835.
** 07-May-2009 (drivi01)
**    In effort to port to Visual Studio 2008
**    clean up the warnings.
*/

/*
** compile:
** esqlc -multi -fsqlcopy.inc sqlcopy.sc
*/


extern int IILQucolinit( int );

/*
** Declare the SQLCA structure and the SQLDA typedef
*/
EXEC SQL INCLUDE SQLCA;
EXEC SQL INCLUDE SQLDA;

static int g_nNewSession4Sequence = 0;
static char* g_strSqlError = (char*)0;

static void FormatSQLError (char* pBuffer)
{


}


char*INGRESII_llGetLastSQLError ()
{
	FormatSQLError (g_strSqlError);
	return g_strSqlError;
}

static int nFirstError = 1;
static void SQLError()
{
	EXEC SQL BEGIN DECLARE SECTION;
		char error_buf[1024];
	EXEC SQL END DECLARE SECTION;

	int nErr = sqlca.sqlcode;
	if (nFirstError == 1)
	{
		nFirstError = 0;
		g_strSqlError = malloc (1024);
	}
	error_buf[0] = '\0';

	EXEC SQL WHENEVER SQLERROR CONTINUE;
	EXEC SQL INQUIRE_INGRES (:error_buf = ERRORTEXT);
	STcopy (error_buf, g_strSqlError);
}

void INGRESII_llCleanSql()
{
	if (!nFirstError && g_strSqlError)
	{
		nFirstError = 1;
		free (g_strSqlError);
		g_strSqlError = NULL;
	}
	if (g_nNewSession4Sequence > 0)
	{
		INGRESII_llDisconnectSession(g_nNewSession4Sequence, 1);
		g_nNewSession4Sequence = 0;
	}
}

int INGRESII_llSetCopyHandler(PfnIISQLHandler handler)
{
	EXEC SQL WHENEVER SQLERROR GOTO SETCOPYHANDLER_LERROR;
	EXEC SQL SET_SQL (COPYHANDLER = handler);
	return 1;
}


int INGRESII_llActivateSession (int nSessionNumer)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int nSession = nSessionNumer;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL WHENEVER SQLERROR GOTO ACTIVATE_ERROR;
	EXEC SQL SET_SQL(SESSION =:nSession);
	return 1;
}

static int nSessionNumber = 0;
int INGRESII_llConnectSession(char* strFullVnode)
{
	EXEC SQL BEGIN DECLARE SECTION;
		char  szSessionDescription[64];
		char* connection = strFullVnode;
		char* o[12]; 
		int   nSession = 1;
		char  unic[10];
	EXEC SQL END DECLARE SECTION;

	o[0] = CMD_GetUFlag(&g_cmd);
	nSessionNumber++;
	nSession = nSessionNumber;
	EXEC SQL WHENEVER SQLERROR GOTO CONNECT_ERROR;
	EXEC SQL SET CONNECTION NONE;
	EXEC SQL SET_SQL( SESSION = NONE );

	if (o[0] && STlength(o[0]) > 0)
		EXEC SQL CONNECT :connection SESSION :nSession OPTIONS = :o[0];
	else
		EXEC SQL CONNECT :connection SESSION :nSession;
	STcopy ("Ingres Data Loader", szSessionDescription);
	EXEC SQL SET SESSION WITH DESCRIPTION = :szSessionDescription;
	EXEC SQL COMMIT;

        EXEC SQL SELECT dbmsinfo('UNICODE_NORMALIZATION') INTO :unic;
        if (STlength(unic) >= 3)
        {
            if (STbcompare( unic, 3, "NFC",3,0) == 0)
            {
              IILQucolinit(1);
            }
            if (STbcompare( unic, 3, "NFD",3,0) == 0)
            {
              IILQucolinit(0);
            }
        }

	return nSession;
CONNECT_ERROR:
	SQLError();
	return 0;
}


int INGRESII_llDisconnectSession(int nSession, int nCommit)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int nSession2Disconnect = nSession;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL WHENEVER SQLERROR GOTO DISCONNECTSESSION_ERROR;
	/*
	** Disconnect the session:
	*/
	if (nSession2Disconnect > 0)
	{
		INGRESII_llActivateSession (nSession);
		if(nCommit == 1)
			EXEC SQL COMMIT;
		EXEC SQL DISCONNECT;
	}
	EXEC SQL SET CONNECTION NONE;
	EXEC SQL SET_SQL( SESSION = NONE );
	return 1;
DISCONNECTSESSION_ERROR:
	SQLError();
	return 0;
}



int INGRESII_llExecuteImmediate (char* strCopyStatement)
{
	EXEC SQL BEGIN DECLARE SECTION;
		char* statement = strCopyStatement;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL WHENEVER SQLERROR GOTO EXECUTEIMMEDIATE_LERROR;
	EXEC SQL EXECUTE IMMEDIATE :statement;
	return 1;

EXECUTEIMMEDIATE_LERROR:
	SQLError();
	return 0;
}

int INGRESII_llExecuteCopyFromProgram (char* strCopyStatement, i4* lAffectRows)
{
	EXEC SQL BEGIN DECLARE SECTION;
		int   nErrorCode = 0;
		int   nCallbackInvoked = 0;
		char* statement = strCopyStatement;
	EXEC SQL END DECLARE SECTION;
	if (lAffectRows)
		*lAffectRows = 0;

	EXEC SQL WHENEVER SQLERROR CONTINUE;
	EXEC SQL EXECUTE IMMEDIATE :statement;

	nCallbackInvoked = g_nGetRecordLineIvoked;
	g_nGetRecordLineIvoked = 0;
	nErrorCode = sqlca.sqlcode;
	switch (nErrorCode)
	{
	case -33000: /* Run-time logical error */
		if (nCallbackInvoked != 1)
		{
			SQLError();
			return 0;
		}
		break;
	default:
		if (nErrorCode < 0)
		{
			SQLError();
			return 0;
		}
		break;
	}

	if (lAffectRows)
		*lAffectRows = (i4)sqlca.sqlerrd[2];
	return 1;
}

void INGRESII_llCommitSequence()
{
	if (g_nNewSession4Sequence <= 0)
		return;
	INGRESII_llActivateSession (g_nNewSession4Sequence);
	EXEC SQL COMMIT;
	INGRESII_llActivateSession (1);
}

int INGRESII_llSequenceNext(char* strSequenceName, char* szNextValue)
{
	EXEC SQL BEGIN DECLARE SECTION;
		char  szStatement[128];
		char  szSeqValue [128];
		char* strName = strSequenceName;
		int   nSess = 0;
	EXEC SQL END DECLARE SECTION;

	szSeqValue[0]='\0';
	STprintf (szStatement, "SELECT VARCHAR (NEXT VALUE FOR %s)", strSequenceName);

	EXEC SQL WHENEVER SQLERROR GOTO NEXT_SEQUENCE_LERROR;
	if (g_nNewSession4Sequence == 0)
	{
		g_nNewSession4Sequence = INGRESII_llConnectSession(CMD_GetVnode(&g_cmd));
		if (g_nNewSession4Sequence == 0)
		{
			SQLError();
			return 0;
		}
	}
	INGRESII_llActivateSession (g_nNewSession4Sequence);

	EXEC SQL EXECUTE IMMEDIATE :szStatement INTO :szSeqValue;
	TrimRight(szSeqValue);
	STcopy (szSeqValue, szNextValue);
	
	INGRESII_llActivateSession (1);
	return 1;
NEXT_SEQUENCE_LERROR:
	SQLError();
	return 0;
}


i4 INGRESII_llGetCPUSession()
{
	EXEC SQL BEGIN DECLARE SECTION;
		char  szResult [64];
		int  lResult;
	EXEC SQL END DECLARE SECTION;

	EXEC SQL WHENEVER SQLERROR GOTO DBMSINFO_LERROR;
	EXEC SQL SELECT DBMSINFO('_cpu_ms') INTO :szResult;
	lResult = atol(szResult);
	return lResult;
DBMSINFO_LERROR:
	SQLError();
	return -1;
}
