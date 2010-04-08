/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijadmlll.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data Manipulation  (Ingres Low level Data Query)
**
** History:
** 23-Dec-1999 (uk$so01)
**    created
** 14-Mar-2000 (noifr01)
**    Cleanup of the CaTemporarySession class for managing more than one
**    concurrent session
** 27-Mar-2000 (noifr01)
**    implemeted the recover/redo syntax for "recover now" and
**    "generate scripts"
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 14-Jun-2001 (noifr01)
**    (bug 104954) commit/rollback immediately after the recover/redo
**    operation, which is needed in order for the later "drop temp. table"
**    management to work correctly 
** 15-Oct-2001 (noifr01)
**    (bug 106041) need to generate "is null" syntax for null columns in the
**    generated scripts
** 14-Mar-2002 (noifr01)
**    (bug 107318) when the "impersonate user[s] of initial transaction[s]"
**    option is used, now commit before the "set session authorization" even
**    before the first real statement, because a side effect of the fix for
**    generating the "is null" syntax is that there may have been a select
**    statement to be committed before "set session authorization" can work
** 09-Jul-2002 (noifr01)
**    (bug 108204) in the generated insert statements, insert a space after
**    the comma between the contents of columns
** 11-Jul-2002 (hanje04)
**    Cannot pass CStrings through %s, cast with (LPCTSTR) in multiple 
**    location where needed.
** 07-Apr-2003 (hanch04)
**    Added a define for tchszReturn to handle return/linefeeds on
**    windows and UNIX.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#include "stdafx.h"
#include "ijadmlll.h"
#include "sqlselec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define MAXDISPLAYSTATEMENTLENGHT 240


CaTemporarySession::CaTemporarySession(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser)
{
	SetIndependent(TRUE);
	m_pSession = NULL;
	SetNode(lpszNode);
	SetDatabase(lpszDatabase);
	if (lpszUser && theApp.m_strAllUser.CompareNoCase(lpszUser) != 0)
		SetUser(lpszUser);
	Open();
}

BOOL CaTemporarySession::IsConnected()
{
	if (!m_pSession)
		return FALSE;
	return m_pSession->IsConnected();
}

BOOL CaTemporarySession::Release(SessionReleaseMode nReleaseMode)
{
	if (!m_pSession)
		return FALSE;
	return m_pSession->Release(nReleaseMode);
}

BOOL CaTemporarySession::Open()
{
	if (!m_bReleased)
		return TRUE;
	if (m_bReleased && m_bConnected)
		return Activate();
	CaSessionManager& smgr = theApp.GetSessionManager();
	SetDescription(smgr.GetDescription());
	m_pSession = smgr.GetSession(this);
	return m_pSession? TRUE: FALSE;
}


CaTemporarySession::~CaTemporarySession()
{
	if (m_pSession)
	{
		m_pSession->Release(SESSION_ROLLBACK);
		m_pSession->Disconnect();
	}
}


// returns -1L if error, otherwise number of rows affected by successful statement
static long Exec1StmtInSession(LPCTSTR lpStatement)
{
	CaExecuteScript param (_T(""), _T(""), lpStatement);
	param.NeedSession(FALSE);
	try
	{
		if (!param.ExecuteStatement(NULL))
			return -1L;
		return param.GetAffectedRows();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
		return -1L;
	}
	catch (...)
	{
		AfxMessageBox (_T("unexpected Error"), MB_OK|MB_ICONEXCLAMATION);
		return -1L;
	}
}

BOOL IJA_RecoverRedoAction (ActionType nType, CaRecoverRedoInformation* pData, CString& strScript, CString& strError, BOOL &bRoundingErrorsPossibleInText)
{
	POSITION pos ,px, posTempTable, poscol, posdata1, posdata2, posCurStmt, posLastExecuted, posLastCommitted;
	CString csStatement, csSubstatement, strColumn,csTempTable,csSelectStatement,strItem1,strItem2;
	CString csTemp, cs1;

	BOOL bFirst,b1,bFirstInWhereClause;
	CaColumn* pColumn;
	unsigned long LSNHigh, LSNLow;
	BOOL bResult = TRUE;
	CStringList listData, listData2;
	int nCount;
	long lRowsAffected;
	CString csCurUsr = _T("");
	BOOL bReversingCommitted = FALSE;
#ifdef MAINWIN
        TCHAR tchszReturn[] = {0x0A, 0x00};
#else
        TCHAR tchszReturn[] = {0x0D, 0x0A, 0x00};
#endif

	strError = _T("");

#if defined (SIMULATION)
	TRACE0 ("IJA_RecoverRedoAction: simulated ...\n");
	return TRUE;
#endif

	BOOL bFirstStatement = TRUE;
	if (pData->m_bNoRule) {
		csStatement = _T("set norules");
		if (nType==ACTION_RECOVER_SCRIPT    ||
		    nType==ACTION_RECOVERNOW_SCRIPT ||
		    nType==ACTION_REDO_SCRIPT       ||
			nType==ACTION_REDONOW_SCRIPT)  {
			strScript = csStatement;
			bFirstStatement = FALSE;
		}
		else {
			CaLowlevelAddAlterDrop param (_T(""), _T(""), csStatement);
			param.NeedSession(FALSE);
			if (!param.ExecuteStatement(NULL)) {
				strError = _T("Cannot execute the \"set norules\" statement");
				bResult = FALSE;
				goto end_recover_redo;
			}
		}
	}

	//
	// pData containts all data for doing Recover / Redo / Script:
	ASSERT (pData);
	if (!pData) {
		//
		// Internal Error
		strError.LoadString(IDS_LOWLEVEL_ERROR_INTERNAL);
		bResult = FALSE;
		goto end_recover_redo;
	}
	posLastExecuted = NULL;
	posLastCommitted= NULL;
	switch (nType) {
		case ACTION_RECOVER:
		case ACTION_RECOVER_SCRIPT:
		case ACTION_RECOVERNOW_SCRIPT:
			pos = pData->m_pListTransaction->GetTailPosition();
			while (pos != NULL) {
recoverloop:	posCurStmt = pos;
				CaBaseTransactionItemData* pBase = pData->m_pListTransaction->GetPrev (pos);
				if (!isUndoRedoable(pBase->GetAction()))   /* don't generate the syntax for non insert/update/delete. User already got a warning */
					continue;
				CString strTable = pBase->GetTable();
				CString strTableOwner = pBase->GetTableOwner();
				pBase->GetLsn(LSNHigh, LSNLow);

				// search for table and associated temporary table
				px = pData->m_listTable.GetHeadPosition();
				posTempTable = pData->m_listTempTable.GetHeadPosition();
				BOOL bFound = FALSE;
				CaIjaTable* pObj;
				CaDBObject *pTempTblInfo;
				while (px != NULL && posTempTable!=NULL) {
					pObj = pData->m_listTable.GetNext (px);
					pTempTblInfo = pData->m_listTempTable.GetNext (posTempTable);
					if (pTempTblInfo->GetOwner().Compare(_T(""))==0) {
						csTempTable=pTempTblInfo->GetItem();
					}
					else {
						csTempTable.Format(_T("%s.%s"),(LPCTSTR)pTempTblInfo->GetOwner(),
													(LPCTSTR)pTempTblInfo->GetItem());
					}
					if (strTable.CompareNoCase (pObj->GetItem()) == 0 && strTableOwner.CompareNoCase (pObj->GetOwner()) == 0) {
						bFound = TRUE;
						break;
					}
				}
				if (!bFound) {
					//
					// Internal Error (with temporary table)\n
					CString strMsg1;
					strMsg1.LoadString (IDS_LOWLEVEL_ERROR_TEMPFILE);

					strError += strMsg1;
					bResult = FALSE;
					break;
				}
				/* 15-Oct-2001 (bug 106041) now get the data even for recover/redo now scripts where a where clause will 
				   be generated, in order to detect nulls - and change the where clauses accordingly in the script (with the
				   "is null" syntax instead of the col= select ... syntax) */
				if (nType==ACTION_RECOVER_SCRIPT || pBase->GetAction() == T_INSERT || pBase->GetAction() == T_AFTERUPDATE) { /* get the data of the row */
					switch (pBase->GetAction()) {
						case T_DELETE:
						case T_INSERT:
						case T_AFTERUPDATE:
							{
								csSelectStatement = _T("select ");
								bFirst = TRUE;
								poscol =  pObj->m_listColumn.GetHeadPosition();
								while (poscol != NULL) {
									pColumn = pObj->m_listColumn.GetNext (poscol);
									strColumn = pColumn->GetName();
									if (!bFirst) 
										csSelectStatement+=_T(",");
									else
										bFirst = FALSE;
									csSelectStatement+=_T("ija_");
									csSelectStatement+=strColumn;
								}
								csSubstatement.Format(_T(" from %s where LSNHIGH=%lu and LSNLOW=%lu"),(LPCTSTR)csTempTable, LSNHigh,LSNLow);
								csSelectStatement+=csSubstatement;
								if (pBase->GetAction()==T_DELETE) /* not really needed (because of the criteria on the LSN) but more consistent */
									csSelectStatement+=_T(" and OPERATION='delete  '");
								if (pBase->GetAction()==T_INSERT) 
									csSelectStatement+=_T(" and ( OPERATION='insert  ' OR  OPERATION='append  ')");
								if (pBase->GetAction()==T_AFTERUPDATE) 
									csSelectStatement+=_T(" and operation='repnew  '");

								CaCursor4Statements cursor(1,csSelectStatement);
								if ( ! cursor.Open()) {
									bResult = FALSE;
									break;
								}

								nCount = 0;
								b1 = cursor.Fetch(listData, nCount);
								cursor.Close();
								if (!b1) {
									bResult = FALSE;
									break;
								}
							}
						default:
							break;
					}
					if (!bResult)
						break;
				}
				switch (pBase->GetAction()) {
					case T_DELETE:
						csStatement.Format(_T("insert into %s.%s("), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
						csSubstatement = _T(") select ");
						bFirst = TRUE;
						poscol =  pObj->m_listColumn.GetHeadPosition();
						while (poscol != NULL) {
							pColumn = pObj->m_listColumn.GetNext (poscol);
							if (pColumn->RoundingErrorsPossibleInText())
								bRoundingErrorsPossibleInText = TRUE;
							strColumn = pColumn->GetName();
							if (!bFirst) {
								csStatement+=_T(",");
								csSubstatement+=_T(",");
							}
							else
								bFirst = FALSE;
							csStatement+=strColumn;
							csSubstatement+=_T("ija_");
							csSubstatement+=strColumn;
						}
						if (nType==ACTION_RECOVER_SCRIPT) {
							csStatement+=_T(") values (");
							bFirst = TRUE;
							posdata1 = listData.GetHeadPosition();
							while (posdata1) {
								strItem1 = listData.GetNext (posdata1);
								if (!bFirst)
									csStatement+=_T(", ");
								else
									bFirst=FALSE;
								csStatement+= strItem1;
							}
							csStatement+=_T(")");
						}
						else {
							csStatement+=csSubstatement;
							csSubstatement.Format(_T(" from %s where LSNHIGH=%lu and LSNLOW=%lu"),(LPCTSTR)csTempTable, LSNHigh,LSNLow);
							csStatement+=csSubstatement;
						}
						break;

					case T_INSERT:
						csStatement.Format(_T("delete from %s.%s where "), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
						bFirst = TRUE;
						poscol =  pObj->m_listColumn.GetHeadPosition();
						posdata1 = listData.GetHeadPosition();
						while (poscol != NULL) {
							pColumn = pObj->m_listColumn.GetNext (poscol);
							if (pColumn->RoundingErrorsPossibleInText())
								bRoundingErrorsPossibleInText = TRUE;
							if (!pColumn->CanBeInWhereClause()) {   // skip those that can't be part of a where clause
								if (nType==ACTION_RECOVER_SCRIPT) { // update position in listData
									if (posdata1==NULL) {
										bResult = FALSE;
										break;
									}
									else 
										strItem1 = listData.GetNext (posdata1);
								}
								continue;
							}
							if (!bFirst) {
								csStatement+=_T(" AND ");
							}
							else
								bFirst = FALSE;
							strColumn = pColumn->GetName();
							if (posdata1==NULL) {
								bResult = FALSE;
								break;
							}
							else 
								strItem1 = listData.GetNext (posdata1);
							if (strItem1.CompareNoCase((LPCTSTR )"null")==0)
								csSubstatement.Format(_T("%s is null"), (LPCTSTR) strColumn);
							else {
								if (nType==ACTION_RECOVER_SCRIPT) {
									csSubstatement.Format(_T("%s=%s"), (LPCTSTR) strColumn, (LPCTSTR) strItem1);
								}
								else {
									csSubstatement.Format(_T("%s=(select ija_%s from %s where LSNHIGH=%lu and LSNLOW=%lu and ( OPERATION='insert  ' OR  OPERATION='append  '))"),
										(LPCTSTR) strColumn,
										(LPCTSTR) strColumn,
										(LPCTSTR)csTempTable,
										 LSNHigh,
										 LSNLow);
								}
							}
							csStatement+=csSubstatement;
						}
						if (!bResult)
							break; // in case additional code is added before next 'break' statement
						break;

					case T_AFTERUPDATE:
						{
							// scan back to the before image: if none: return an error
							if (pos==NULL) {
								bResult = FALSE;
								break;
							}
							posCurStmt = pos;
							CaBaseTransactionItemData* pBaseBefImage = pData->m_pListTransaction->GetPrev (pos);
							CString strTableBef = pBaseBefImage->GetTable();
							CString strTableBefOwner = pBaseBefImage->GetTableOwner();
							unsigned long lLsnBefHigh, lLsnBefLow;
							pBaseBefImage->GetLsn(lLsnBefHigh, lLsnBefLow);
							if (strTableBef.Compare(strTable)!=0 || strTableBefOwner.Compare(strTableOwner)!=0 ||
								pBaseBefImage->GetAction()!=T_BEFOREUPDATE ||
								lLsnBefHigh!=LSNHigh || lLsnBefLow!=LSNLow) {
								bResult = FALSE;
								break;
							}
							/* now execute the select statement also for "recover now" scripts, in order */
							/* to detect null values (and change the where clause accordingly)      */
							if (TRUE /*nType==ACTION_RECOVER_SCRIPT*/) {
								csSelectStatement = _T("select ");
								bFirst = TRUE;
								poscol =  pObj->m_listColumn.GetHeadPosition();
								while (poscol != NULL) {
									pColumn = pObj->m_listColumn.GetNext (poscol);
									strColumn = pColumn->GetName();
									if (!bFirst) 
										csSelectStatement+=_T(",");
									else
										bFirst = FALSE;
									csSelectStatement+=_T("ija_");
									csSelectStatement+=strColumn;
								}
								csSubstatement.Format(_T(" from %s where LSNHIGH=%lu and LSNLOW=%lu and operation='repold  '"),(LPCTSTR)csTempTable, lLsnBefHigh,lLsnBefLow);
								csSelectStatement+=csSubstatement;

								CaCursor4Statements cursor(2,csSelectStatement);
								if ( ! cursor.Open()) {
									bResult = FALSE;
									break;
								}

								nCount = 0;
								b1 = cursor.Fetch(listData2, nCount);
								cursor.Close();
								if (!b1) {
									bResult = FALSE;
									break;
								}
							}
							if (nType==ACTION_RECOVER_SCRIPT) {
								csStatement.Format(_T("update %s.%s  set "),
													(LPCTSTR)strTableOwner,
													(LPCTSTR)strTable);
								csSubstatement=(_T(" where "));
							}
							else {
								csStatement.Format(_T("update %s.%s from %s ija____temp_tbl1, %s ija____temp_tbl2  set "),
													(LPCTSTR)strTableOwner,
													(LPCTSTR)strTable,
													(LPCTSTR)csTempTable,
													(LPCTSTR)csTempTable);
								csSubstatement.Format(_T(" where ija____temp_tbl1.LSNHIGH=%lu and ija____temp_tbl1.LSNLOW=%lu and ija____temp_tbl2.LSNHIGH=%lu and ija____temp_tbl2.LSNLOW=%lu and "), LSNHigh, LSNLow, LSNHigh, LSNLow);
								csSubstatement+=_T("ija____temp_tbl1.OPERATION='repold  ' and ija____temp_tbl2.OPERATION='repnew  ' and ");
							}
							bFirst = TRUE;
							bFirstInWhereClause = TRUE;
							poscol =  pObj->m_listColumn.GetHeadPosition();
							posdata1 = listData.GetHeadPosition();
							posdata2 = listData2.GetHeadPosition();
							while (poscol != NULL) {
								pColumn = pObj->m_listColumn.GetNext (poscol);
								if (pColumn->RoundingErrorsPossibleInText())
									bRoundingErrorsPossibleInText = TRUE;
								strColumn = pColumn->GetName();

								if (pColumn->CanBeInWhereClause()) {  
									if (!bFirstInWhereClause) 
										csSubstatement+=_T(" AND ");
									else
										bFirstInWhereClause = FALSE;
								}
								if (!bFirst) 
									csStatement+=_T(",");
								else
									bFirst = FALSE;

								if (posdata1==NULL || posdata2==NULL) {
									bResult = FALSE;
									break;
								}
								else  {
									strItem1 = listData.GetNext (posdata1);
									strItem2 = listData.GetNext (posdata2);
								}

								if (nType==ACTION_RECOVER_SCRIPT) {
									csStatement+=strColumn;
									csStatement+=_T("=");
									csStatement+=strItem2;

									if (pColumn->CanBeInWhereClause()) {  
										if (strItem1.CompareNoCase((LPCTSTR )"null")==0) {
											csSubstatement+=strColumn;
											csSubstatement+= _T(" is null");
										}
										else {
											csSubstatement+=strColumn;
											csSubstatement+=_T("=");
											csSubstatement+=strItem1;
										}
									}
								}
								else {
									csStatement+=strColumn;
									csStatement+=_T("=ija____temp_tbl1.ija_");
									csStatement+=strColumn;

									if (pColumn->CanBeInWhereClause()) {  
										if (strItem1.CompareNoCase((LPCTSTR )"null")==0) {
											csSubstatement+=strColumn;
											csSubstatement+= _T(" is null");
										}
										else {
											csSubstatement+=strColumn;
											csSubstatement+=_T("=ija____temp_tbl2.ija_");
											csSubstatement+=strColumn;
										}
									}
								}
							}
							if (!bResult)
								break;
							csStatement+=csSubstatement;
						}
						break;

					case T_BEFOREUPDATE:
						bResult = FALSE; /* impossible, because normally scanned (back from the code for the after image)*/
						break;

					default:
						bResult = FALSE;
						break;

				}
				listData.RemoveAll();
				listData2.RemoveAll();
				if (!bResult)
					break;
				else {
					if (nType!=ACTION_RECOVER) {
						// manage user impersonation
						if (pData->m_bImpersonateUser) {
							if ( csCurUsr.CompareNoCase(pBase->GetUser()) !=0) {
								CString csImpersonate;
								csCurUsr=pBase->GetUser();
								csImpersonate.Format(_T("set session authorization %s"),(LPCTSTR)csCurUsr);
								if (bFirstStatement) {
									strScript = csImpersonate;
									bFirstStatement = FALSE;
								}
								else {
									strScript+=_T(";");
									strScript+= tchszReturn;
									strScript+=_T("commit;");
									strScript+= tchszReturn;
									strScript+=csImpersonate;
								}
							}
						}
						if (bFirstStatement) {
							strScript = csStatement;
							bFirstStatement = FALSE;
						}
						else {
							strScript+=_T(";");
							strScript+= tchszReturn;
							strScript+=csStatement;
						}
						continue;
					}
					if (pData->m_bImpersonateUser) {
						if ( csCurUsr.CompareNoCase(pBase->GetUser()) !=0) {
							CString csImpersonate;
							csCurUsr=pBase->GetUser();
							csImpersonate.Format(_T("set session authorization %s"),(LPCTSTR)csCurUsr);
							if (bFirstStatement) {
								/* commit before the "set session authorization" even before the first
								   real statement, because a side effect of the fix for generating the 
								   "is null" syntax is that there may have been a select statement to 
								   be committed before "set session authorization" can work */
								lRowsAffected = Exec1StmtInSession((LPCTSTR) _T("commit"));
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
								posLastCommitted = posLastExecuted; /* normally NULL anyhow in this case */
								lRowsAffected = Exec1StmtInSession((LPCTSTR) csImpersonate);
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
								bFirstStatement = FALSE;
							}
							else {
								lRowsAffected = Exec1StmtInSession((LPCTSTR) _T("commit"));
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
								posLastCommitted = posLastExecuted; /* remains NULL if only "set norules" has been executed, in which case, nothing to undo */
								lRowsAffected = Exec1StmtInSession((LPCTSTR) csImpersonate);
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
							}
						}
					}
					lRowsAffected = Exec1StmtInSession((LPCTSTR)csStatement);
					if (lRowsAffected <0L) {
						bResult = FALSE;
						break;
					}
					if (lRowsAffected !=1L) {
						CString csShortStatement;
						if (csStatement.GetLength()>MAXDISPLAYSTATEMENTLENGHT)
							csShortStatement=csStatement.Left(MAXDISPLAYSTATEMENTLENGHT -3) + _T("...");
						else
							csShortStatement=csStatement;
						if ( lRowsAffected == 0L) {
							//
							// No existing row has been affected by the following Undo Statement:\n%s\n
							CString strFormat;
							strFormat.LoadString (IDS_LOWLEVEL_ERROR_1);

							csTemp.Format(strFormat, (LPCTSTR)csShortStatement);
							strError+=csTemp;
							bResult = FALSE;
							break;
						}
						if ( lRowsAffected > 1L && pData->m_bErrorOnAffectedRowCount) {
							//
							// More than one row has been affected by the following Undo Statement:\n%s\n
							CString strFormat;
							strFormat.LoadString (IDS_LOWLEVEL_ERROR_2);
							csTemp.Format(strFormat, (LPCTSTR)csShortStatement);
							strError+=csTemp;
							bResult = FALSE;
							break;
						}
					}
					posLastExecuted = posCurStmt;
				}  //else
			}
			break;
		case ACTION_REDO:
		case ACTION_REDO_SCRIPT:
		case ACTION_REDONOW_SCRIPT:
			pos = pData->m_pListTransaction->GetHeadPosition();
			while (pos != NULL) {
redoloop:		posCurStmt = pos;
				CaBaseTransactionItemData* pBase = pData->m_pListTransaction->GetNext (pos);
				if (!isUndoRedoable(pBase->GetAction()))   /* don't generate the syntax for non insert/update/delete. User already got a warning */
					continue;
				CString strTable = pBase->GetTable();
				CString strTableOwner = pBase->GetTableOwner();
				pBase->GetLsn(LSNHigh, LSNLow);

				// search for table and associated temporary table
				px = pData->m_listTable.GetHeadPosition();
				posTempTable = pData->m_listTempTable.GetHeadPosition();
				BOOL bFound = FALSE;
				CaIjaTable* pObj;
				CaDBObject *pTempTblInfo;
				while (px != NULL && posTempTable!=NULL) {
					pObj = pData->m_listTable.GetNext (px);
					pTempTblInfo = pData->m_listTempTable.GetNext (posTempTable);
					if (pTempTblInfo->GetOwner().Compare(_T(""))==0) {
						csTempTable=pTempTblInfo->GetItem();
					}
					else {
						csTempTable.Format(_T("%s.%s"),(LPCTSTR)pTempTblInfo->GetOwner(),
													(LPCTSTR)pTempTblInfo->GetItem());
					}
					if (strTable.CompareNoCase (pObj->GetItem()) == 0 && strTableOwner.CompareNoCase (pObj->GetOwner()) == 0) {
						bFound = TRUE;
						break;
					}
				}
				if (!bFound) {
					bResult = FALSE;
					break;
				}
				if (nType==ACTION_REDO_SCRIPT  || pBase->GetAction() == T_DELETE || pBase->GetAction() == T_BEFOREUPDATE) { /* get the data of the row (rather than using select statements as an input to the final query)*/
					switch (pBase->GetAction()) {
						case T_DELETE:
						case T_INSERT:
						case T_BEFOREUPDATE:
							{
								csSelectStatement = _T("select ");
								bFirst = TRUE;
								poscol =  pObj->m_listColumn.GetHeadPosition();
								while (poscol != NULL) {
									pColumn = pObj->m_listColumn.GetNext (poscol);
									strColumn = pColumn->GetName();
									if (!bFirst) 
										csSelectStatement+=_T(",");
									else
										bFirst = FALSE;
									csSelectStatement+=_T("ija_");
									csSelectStatement+=strColumn;
								}
								csSubstatement.Format(_T(" from %s where LSNHIGH=%lu and LSNLOW=%lu"),(LPCTSTR)csTempTable, LSNHigh,LSNLow);
								csSelectStatement+=csSubstatement;
								if (pBase->GetAction()==T_DELETE) /* not really needed (because of the criteria on the LSN) but more consistent */
									csSelectStatement+=_T(" and OPERATION='delete  '");
								if (pBase->GetAction()==T_INSERT) 
									csSelectStatement+=_T(" and ( OPERATION='insert  ' OR  OPERATION='append  ')");
								if (pBase->GetAction()==T_BEFOREUPDATE) 
									csSelectStatement+=_T(" and operation='repold  '");

								CaCursor4Statements cursor(1,csSelectStatement);
								if ( ! cursor.Open()) {
									bResult = FALSE;
									break;
								}

								nCount = 0;
								b1 = cursor.Fetch(listData, nCount);
								cursor.Close();
								if (!b1) {
									bResult = FALSE;
									break;
								}
							}
						default:
							break;
					}
					if (!bResult)
						break;
				}
				switch (pBase->GetAction()) {
					case T_DELETE:
						csStatement.Format(_T("delete from %s.%s where "), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
						bFirst = TRUE;
						poscol =  pObj->m_listColumn.GetHeadPosition();
						posdata1 = listData.GetHeadPosition();
						while (poscol != NULL) {
							pColumn = pObj->m_listColumn.GetNext (poscol);
							if (pColumn->RoundingErrorsPossibleInText())
								bRoundingErrorsPossibleInText = TRUE;
							if (!pColumn->CanBeInWhereClause()) {   // skip those that can't be part of a where clause
								if (nType==ACTION_REDO_SCRIPT) { // update position in listData
									if (posdata1==NULL) {
										bResult = FALSE;
										break;
									}
									else 
										strItem1 = listData.GetNext (posdata1);
								}
								continue;
							}
							strColumn = pColumn->GetName();
							if (!bFirst) {
								csStatement+=_T(" AND ");
							}
							else
								bFirst = FALSE;

							if (posdata1==NULL) {
								bResult = FALSE;
								break;
							}
							else 
								strItem1 = listData.GetNext (posdata1);
							if (strItem1.CompareNoCase((LPCTSTR )"null")==0)
								csSubstatement.Format(_T("%s is null"), (LPCTSTR) strColumn);
							else {
								if (nType==ACTION_REDO_SCRIPT) {
									csSubstatement.Format(_T("%s=%s"), (LPCTSTR) strColumn, (LPCTSTR) strItem1);
								}
								else {
									csSubstatement.Format(_T("%s=(select ija_%s from %s where LSNHIGH=%lu and LSNLOW=%lu and OPERATION='delete  ')"),
										(LPCTSTR) strColumn,
										(LPCTSTR) strColumn,
										(LPCTSTR)csTempTable,
										 LSNHigh,
										 LSNLow);
								}
							}
							csStatement+=csSubstatement;
						}
						if (!bResult)
							break; // in case additional code is added before next 'break' statement
						break;

					case T_INSERT:
						csStatement.Format(_T("insert into %s.%s("), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
						csSubstatement = _T(") select ");
						bFirst = TRUE;
						poscol =  pObj->m_listColumn.GetHeadPosition();
						while (poscol != NULL) {
							pColumn = pObj->m_listColumn.GetNext (poscol);
							if (pColumn->RoundingErrorsPossibleInText())
								bRoundingErrorsPossibleInText = TRUE;
							strColumn = pColumn->GetName();
							if (!bFirst) {
								csStatement+=_T(",");
								csSubstatement+=_T(",");
							}
							else
								bFirst = FALSE;
							csStatement+=strColumn;
							csSubstatement+=_T("ija_");
							csSubstatement+=strColumn;
						}
						if (nType==ACTION_REDO_SCRIPT) {
							csStatement+=_T(") values (");
							bFirst = TRUE;
							posdata1 = listData.GetHeadPosition();
							while (posdata1) {
								strItem1 = listData.GetNext (posdata1);
								if (!bFirst)
									csStatement+=_T(", ");
								else
									bFirst=FALSE;
								csStatement+= strItem1;
							}
							csStatement+=_T(")");
						}
						else {
							csStatement+=csSubstatement;
							csSubstatement.Format(_T(" from %s where LSNHIGH=%lu and LSNLOW=%lu"),(LPCTSTR)csTempTable, LSNHigh,LSNLow);
							csStatement+=csSubstatement;
						}
						break;


					case T_BEFOREUPDATE:
						{
							// scan back to the before image: if none: return an error
							if (pos==NULL) {
								bResult = FALSE;
								break;
							}
							posCurStmt = pos;
							CaBaseTransactionItemData* pBaseAfterImage = pData->m_pListTransaction->GetNext (pos);
							CString strTableBef = pBaseAfterImage->GetTable();
							CString strTableBefOwner = pBaseAfterImage->GetTableOwner();
							unsigned long lLsnAfterfHigh, lLsnAfterLow;
							pBaseAfterImage->GetLsn(lLsnAfterfHigh, lLsnAfterLow);
							if (strTableBef.Compare(strTable)!=0 || strTableBefOwner.Compare(strTableOwner)!=0 ||
								pBaseAfterImage->GetAction()!=T_AFTERUPDATE ||
								lLsnAfterfHigh!=LSNHigh || lLsnAfterLow!=LSNLow) {
								bResult = FALSE;
								break;
							}

							/* now execute the select statement also for "redo now" scripts, in order */
							/* to detect null values (and change the where clause accordingly)      */
							if (TRUE /*nType==ACTION_REDO_SCRIPT*/ ) {
								csSelectStatement = _T("select ");
								bFirst = TRUE;
								poscol =  pObj->m_listColumn.GetHeadPosition();
								while (poscol != NULL) {
									pColumn = pObj->m_listColumn.GetNext (poscol);
									strColumn = pColumn->GetName();
									if (!bFirst) 
										csSelectStatement+=_T(",");
									else
										bFirst = FALSE;
									csSelectStatement+=_T("ija_");
									csSelectStatement+=strColumn;
								}
								csSubstatement.Format(_T(" from %s where LSNHIGH=%lu and LSNLOW=%lu and operation='repnew  '"),(LPCTSTR)csTempTable, lLsnAfterfHigh,lLsnAfterLow);
								csSelectStatement+=csSubstatement;

								CaCursor4Statements cursor(2,csSelectStatement);
								if ( ! cursor.Open()) {
									bResult = FALSE;
									break;
								}

								nCount = 0;
								b1 = cursor.Fetch(listData2, nCount);
								cursor.Close();
								if (!b1) {
									bResult = FALSE;
									break;
								}
							}
							if (nType==ACTION_REDO_SCRIPT) {
								csStatement.Format(_T("update %s.%s  set "),
													(LPCTSTR)strTableOwner,
													(LPCTSTR)strTable);
								csSubstatement=(_T(" where "));
							}
							else {
								csStatement.Format(_T("update %s.%s from %s ija____temp_tbl1, %s ija____temp_tbl2  set "),
												(LPCTSTR)strTableOwner,
												(LPCTSTR)strTable,
												(LPCTSTR)csTempTable,
												(LPCTSTR)csTempTable);
								csSubstatement.Format(_T(" where ija____temp_tbl1.LSNHIGH=%lu and ija____temp_tbl1.LSNLOW=%lu and ija____temp_tbl2.LSNHIGH=%lu and ija____temp_tbl2.LSNLOW=%lu and "), LSNHigh, LSNLow, LSNHigh, LSNLow);
								csSubstatement+=_T("ija____temp_tbl1.OPERATION='repold  ' and ija____temp_tbl2.OPERATION='repnew  ' and ");
							}
							bFirst = TRUE;
							bFirstInWhereClause = TRUE;
							poscol =  pObj->m_listColumn.GetHeadPosition();
							posdata1 = listData.GetHeadPosition();
							posdata2 = listData2.GetHeadPosition();
							while (poscol != NULL) {
								pColumn = pObj->m_listColumn.GetNext (poscol);
								if (pColumn->RoundingErrorsPossibleInText())
									bRoundingErrorsPossibleInText = TRUE;
								strColumn = pColumn->GetName();
								if (pColumn->CanBeInWhereClause()) {  
									if (!bFirstInWhereClause) 
										csSubstatement+=_T(" AND ");
									else
										bFirstInWhereClause = FALSE;
								}
								if (!bFirst)
									csStatement+=_T(",");
								else
									bFirst = FALSE;

								if (posdata1==NULL || posdata2==NULL) {
									bResult = FALSE;
									break;
								}
								else  {
									strItem1 = listData.GetNext (posdata1);
									strItem2 = listData.GetNext (posdata2);
								}

								if (nType==ACTION_REDO_SCRIPT) {
									csStatement+=strColumn;
									csStatement+=_T("=");
									csStatement+=strItem2;

									if (pColumn->CanBeInWhereClause()) {
										if (strItem1.CompareNoCase((LPCTSTR )"null")==0) {
											csSubstatement+=strColumn;
											csSubstatement+= _T(" is null");
										}
										else {
											csSubstatement+=strColumn;
											csSubstatement+=_T("=");
											csSubstatement+=strItem1;
										}
									}
								}
								else {
									csStatement+=strColumn;
									csStatement+=_T("=ija____temp_tbl2.ija_");
									csStatement+=strColumn;

									if (pColumn->CanBeInWhereClause()) {
										if (strItem1.CompareNoCase((LPCTSTR )"null")==0) {
											csSubstatement+=strColumn;
											csSubstatement+= _T(" is null");
										}
										else {
											csSubstatement+=strColumn;
											csSubstatement+=_T("=ija____temp_tbl1.ija_");
											csSubstatement+=strColumn;
										}
									}
								}

							}
							if (!bResult)
								break;
							csStatement+=csSubstatement;
						}
						break;

					case T_AFTERUPDATE:
						bResult = FALSE; /* impossible, because normally scanned (back from the code for the after image)*/
						break;

					default:
						bResult = FALSE;
						break;

				}
				listData.RemoveAll();
				listData2.RemoveAll();
				if (!bResult)
					break;
				else {
					if (nType!=ACTION_REDO) {
						// manage user impersonation
						if (pData->m_bImpersonateUser) {
							if ( csCurUsr.CompareNoCase(pBase->GetUser()) !=0) {
								CString csImpersonate;
								csCurUsr=pBase->GetUser();
								csImpersonate.Format(_T("set session authorization %s"),(LPCTSTR)csCurUsr);
								if (bFirstStatement) {
									strScript = csImpersonate;
									bFirstStatement = FALSE;
								}
								else {
									strScript+=_T(";");
									strScript+=tchszReturn;
									strScript+=_T("commit;");
									strScript+=tchszReturn;
									strScript+=csImpersonate;
								}
							}
						}
						if (bFirstStatement) {
							strScript = csStatement;
							bFirstStatement = FALSE;
						}
						else {
							strScript+=_T(";");
							strScript+=tchszReturn;
							strScript+=csStatement;
						}
						continue;
					}
					if (pData->m_bImpersonateUser) {
						if ( csCurUsr.CompareNoCase(pBase->GetUser()) !=0) {
							CString csImpersonate;
							csCurUsr=pBase->GetUser();
							csImpersonate.Format(_T("set session authorization %s"),(LPCTSTR)csCurUsr);
							if (bFirstStatement) {
								/* commit before the "set session authorization" even before the first
								   real statement, because a side effect of the fix for generating the 
								   "is null" syntax is that there may have been a select statement to 
								   be committed before "set session authorization" can work */
								lRowsAffected = Exec1StmtInSession((LPCTSTR) _T("commit"));
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
								posLastCommitted = posLastExecuted; /* normally NULL anyhow in this case */
								lRowsAffected = Exec1StmtInSession((LPCTSTR) csImpersonate);
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
								bFirstStatement = FALSE;
							}
							else {
								lRowsAffected = Exec1StmtInSession((LPCTSTR) _T("commit"));
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
								posLastCommitted = posLastExecuted; /* remains NULL if only "set norules" has been executed, in which case, nothing to undo */
								lRowsAffected = Exec1StmtInSession((LPCTSTR) csImpersonate);
								if (lRowsAffected<0L) {
									bResult = FALSE;
									break;
								}
							}
						}
					}
					lRowsAffected = Exec1StmtInSession((LPCTSTR)csStatement);
					if (lRowsAffected <0L) {
						bResult = FALSE;
						break;
					}
					if (lRowsAffected !=1L) {
						CString csShortStatement;
						if (csStatement.GetLength()>MAXDISPLAYSTATEMENTLENGHT)
							csShortStatement=csStatement.Left(MAXDISPLAYSTATEMENTLENGHT -3) + _T("...");
						else
							csShortStatement=csStatement;
						if ( lRowsAffected == 0L) {
							//
							// No existing row has been affected by the following Redo Statement:\n%s\n
							CString strFormat;
							strFormat.LoadString (IDS_LOWLEVEL_ERROR_3);

							csTemp.Format(strFormat, (LPCTSTR)csShortStatement);
							strError+=csTemp;
							bResult = FALSE;
							break;
						}
						if ( lRowsAffected > 1L && pData->m_bErrorOnAffectedRowCount) {
							//
							// More than one row has been affected by the following Redo Statement:\n%s\n
							CString strFormat;
							strFormat.LoadString (IDS_LOWLEVEL_ERROR_4);

							csTemp.Format(strFormat, (LPCTSTR)csShortStatement);
							strError+=csTemp;
							bResult = FALSE;
							break;
						}
					}
					posLastExecuted = posCurStmt;
				}  // else
			}
			break;
		default:
			break;
	}

	if (!bResult && !bReversingCommitted && posLastCommitted) {
		lRowsAffected = Exec1StmtInSession((LPCTSTR) _T("rollback"));
		if (lRowsAffected<0L) {
			if (nType == ACTION_RECOVER) {
				// \n\nWarning: some recover statements already have been committed (because you have asked to recover more than one transaction, and to 
				// impersonate the initial users of the transactions).\n
				// You will need to cancel these manually (typically with IJA), because right now there is a problem executing a query in this session, 
				// therefore IJA doesn't propose to try to do it automatically.

				cs1.LoadString(IDS_LOWLEVEL_ERROR_5);
			}
			else {
				// \n\nWarning: some redo statements already have been committed (because you have asked to redo more than one transaction, and to 
				// impersonate the initial users of the transactions).\n
				// You will need to cancel these manually (typically with IJA), because right now there is a problem executing a query in this session,
				// therefore IJA doesn't propose to try to do it automatically.

				cs1.LoadString(IDS_LOWLEVEL_ERROR_6);
			}
			strError +=cs1;
		}
		else {
			CString strMsg;
			CString strNewMsg;
			if (nType == ACTION_RECOVER) {
				// \nError after some recover statements already have been committed (because you have asked to recover more than one transaction, and to 
				// impersonate the initial users of the transactions).\n\n
				// Do you want to undo those recover statements that already have been committed ?\n\n
				// IMPORTANT: this will be done with the same settings for the "NORULES", "impersonate user[s] of initial transactions", and "management 
				// if >1 row is affected by a single statement" options as what was used for the recover operation.\nPlease choose NO and use IJA manually if you want 
				// to use different settings.

				strNewMsg.LoadString (IDS_LOWLEVEL_ERROR_7);
			}
			else {
				// \nError after some redo statements already have been committed (because you have asked to redo more than one transaction, and to 
				// impersonate the initial users of the transactions).\n\n
				// Do you want to undo those redo statements that already have been committed ?\n\n
				// IMPORTANT: this will be done with the same settings for the "NORULES", "impersonate user[s] of initial transactions, and "management 
				// if >1 row is affected by a single statement" options as what was used for the redo operation.\nPlease choose NO and use IJA manually if you want 
				// to use different settings.

				strNewMsg.LoadString (IDS_LOWLEVEL_ERROR_8);
			}
			strMsg = strError + strNewMsg;
			int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
			if (nAnswer == IDYES) {
				bReversingCommitted = TRUE;
				pos = posLastCommitted;
				csCurUsr = _T("");
				bResult = TRUE;
				if (nType == ACTION_RECOVER) {
					nType = ACTION_REDO;
					goto redoloop;
				}
				else {
					nType = ACTION_RECOVER;
					goto recoverloop;
				}
			}
			else {
				if (nType == ACTION_RECOVER) {
					// \n\nSome recover statements already have been committed (because you have asked to recover more than one transaction, and to 
					// impersonate the initial users of the transactions).\nYou have asked not to undo those statements.
				
					cs1.LoadString(IDS_LOWLEVEL_ERROR_9);
				}
				else {
					// \n\nSome redo statements already have been committed (because you have asked to redo more than one transaction, and to 
					// impersonate the initial users of the transactions).\nYou have asked not to undo those statements.

					cs1.LoadString(IDS_LOWLEVEL_ERROR_10);
				}
				strError +=cs1;
			}
		}
	}

	if (bReversingCommitted && bResult) {
		lRowsAffected = Exec1StmtInSession((LPCTSTR) _T("commit")); 
		if (lRowsAffected<0L) {
			bResult = FALSE;
			//
			// Failed to commit the undo of a transaction that had been committed before the initial error\n.
			cs1.LoadString(IDS_LOWLEVEL_ERROR_11);
			strError += cs1;
		}
	}

	if (bReversingCommitted) {
		if (bResult) { // undoing what already had been committed has worked, but the global operation didn't succeed
			bResult = FALSE;
			if (nType == ACTION_RECOVER) { // the initial operation was a REDO
				// \n\n[ Before the failure, some "redo" transactions had been committed (because you have asked to redo more than one transaction, and to 
				// impersonate the initial users of the transactions), but you have asked to undo them (using the same options as what was chosen for the redo operation), 
				// which has been successful. ]

				cs1.LoadString(IDS_LOWLEVEL_ERROR_12);
			}
			else {  // the initial operation was a RECOVER
				// \n\n[ Before the failure, some "recover" transactions had been committed (because you have asked to recover more than one transaction, and to 
				// impersonate the initial users of the transactions), but you have asked to undo them (using the same options as what was chosen for the recover operation), 
				// which has been successful. ]

				cs1.LoadString(IDS_LOWLEVEL_ERROR_13);
			}
			strError +=cs1;
		}
		else {
			if (nType == ACTION_RECOVER) { // the initial operation was a REDO
				// \n\nBefore the failure, some "redo" transactions had been committed (because you have asked to redo more than one transaction, and to 
				// impersonate the initial users of the transactions).\nYou have asked to undo them (using the same options as what was chosen for the redo operation), 
				// and this has failed also.
			
				cs1.LoadString(IDS_LOWLEVEL_ERROR_14);
			}
			else {  // the initial operation was a RECOVER
				// \n\nBefore the failure, some "recover" transactions had been committed (because you have asked to recover more than one transaction, and to 
				// impersonate the initial users of the transactions).\nYou have asked to undo them (using the same options as what was chosen for the recover operation), 
				// and this has failed also.

				cs1.LoadString(IDS_LOWLEVEL_ERROR_15);
			}
			strError +=cs1;
		}
	}
end_recover_redo:
	/* the commit or rollback is now already done here, because: */
	/* - a commit will be needed anyhow for dropping the temp.   */
	/*   table, even if the recover/redo statements are to be    */
	/*   rollbacked. Therefore the potential rollback is to be   */
	/*   first, i.e. here)                                       */
	/* - also because of the "impersonation" feature, later      */
	/*   "set session authorization" statements (used for        */
	/*   dropping the temp table(s) ) wouldn't work if the       */
	/*   current transaction is not committed or rollbacked      */
	if (bResult)
		cs1 = _T("commit");
	else
		cs1 = _T("rollback");
	lRowsAffected = Exec1StmtInSession((LPCTSTR) cs1);
	if (lRowsAffected<0L) { /* normally, a messagebox already has occured at this point */
		strError += _T("\nError in final ");
		strError += (LPCTSTR) cs1;
		bResult = FALSE;
	}
	return bResult;
}

#define ESQLCC_UNICODE_OK
#if defined (_UNICODE)
//
// In actual ESCLCC, we cannot use the TCHAR data type, there are some casts LPTSTR to char*
// If later, Ingres supports the UNICODE, then use the correct data type and remove the casts...
// Remove the #error macro if Ingres supports UNICODE
#error Ingres ESQLCC is not UNICODE compliant, the instruction bellow #include "ijadmlll.inc" will generate compile error
#undef ESQLCC_UNICODE_OK // Remove this line if Ingres supports UNICODE
#endif
//
// This file "ijadmlll.inc" is generated from ESQLCC -multi -fijadmlll.inc ijadmlll.scc
#if defined ESQLCC_UNICODE_OK
#include "ijadmlll.inc"
#endif



