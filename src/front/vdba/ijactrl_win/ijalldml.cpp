/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijalldml.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01),
**
**    Purpose  : Low level Data Manipulation
**
** History:
** 22-Dec-1999 (uk$so01)
**    Created
** 13/16-Mar-2000 (noifr01)
**    updated for remote management
** 04-May-2001 (noifr01)
**    (ija counterpart of bug 103109) added the new -expand_lobjs flag 
**    in the auditdb command line at the database level
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 16-May-2001 (noifr01)
**    (bug 104729) don't try to do a query in the temporary table if
**    the function that was supposed to populate it returned an error..
**    Also cleaned up the message in case of E_DM1601_FLOAD_OPEN_DB
**    error from adbtofst
** 29-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Management the quote string and wording.
** 14-Jun-2001 (noifr01)
**    (bug 104954) manage the grant on the temporary tables
** 10-Oct-2001 (hanje04)
**    Bug:105483
**    Added m_strLocalIITemporary so that strLocalIngresTemp doesn't have to
**    be constructed from m_strLocalIISystem, which is wrong for UNIX. This will
**    be set to II_TEMPORARY as it should be.
** 08-Mar-2002 (noifr01)
**    (bug 107292)
**    removed unused GenerateFastLoadSyntax() function
** 10-Jun-2002 (hanje04)
**    (Bug 107970)
**    In GenerateDBLevelAuditdbInfo cast parameters to AdbToFstSyntax.Format 
**    (LPCTSTR) to stop Syntax errors on Unix.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 15-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/

#include "stdafx.h"
#include "ijactrl.h"
#include "ijalldml.h"
#include "ijadmlll.h"
#include "ijactdml.h"
#include "sqlselec.h"
#include "parser.h"
#include "rcrdtool.h"
#include "remotecm.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static BOOL DATABASE_AuditdbOutput(LPCTSTR lpszAuditDb, CString& strOutput);

static CaTableTransactionItemData* TRANSACTION_Find (
    unsigned long txHigh, 
    unsigned long txLow, 
    CTypedPtrList < CObList, CaTableTransactionItemData* >& ls)
{
	unsigned long lLow, lHigh;
	CaTableTransactionItemData* pObj = NULL;
	POSITION pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		pObj = ls.GetNext (pos);
		pObj->GetTransactionID (lHigh, lLow);
		if (txHigh == lHigh && txLow == lLow)
			return pObj;
	}

	return NULL;
}

static void TRANSACTION_ChangeState (
    unsigned long txHigh, 
    unsigned long txLow, 
    CString& strOperation,
    CTypedPtrList < CObList, CaTableTransactionItemData* >& ls)
{
	unsigned long lLow, lHigh;
	CaTableTransactionItemData* pObj = NULL;
	POSITION pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		pObj = ls.GetNext (pos);
		pObj->GetTransactionID (lHigh, lLow);
		if (txHigh == lHigh && txLow == lLow)
		{
			if (strOperation.CompareNoCase (_T("COMMIT")) == 0)
			{
				pObj->SetCommit  (TRUE);
				pObj->SetJournal (TRUE);
			}
			else
			if (strOperation.CompareNoCase (_T("ABORT")) == 0)
			{
				pObj->SetCommit  (FALSE);
				pObj->SetJournal (TRUE);
			}
			else
			if (strOperation.CompareNoCase (_T("COMMITNJ")) == 0)
			{
				pObj->SetCommit  (TRUE);
				pObj->SetJournal (FALSE);
			}
			else
			if (strOperation.CompareNoCase (_T("ABORTNJ")) == 0)
			{
				pObj->SetCommit  (FALSE);
				pObj->SetJournal (FALSE);
			}
			else
			{
				//
				// String in OPERATION in not in ('commit', 'abort', 'commitnj', 'abortnj')
				ASSERT (FALSE);
			}
		}
	}
}

static void GenerateDBLevelAuditdbSyntax(CaQueryTransactionInfo* pQueryInfo,  CString& strSyntax, LPCTSTR ConnectedUser)
{
	CString strDatabase;
	CString strDatabaseOwner;
	CString strTableOwner;

	pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);

	CString strFlags = _T("");
	if (pQueryInfo->GetInconsistent())
		strFlags+= _T("-inconsistent ");
	if (pQueryInfo->GetWait())
		strFlags+= _T("-wait ");
	CString strUser = pQueryInfo->GetUser();
	if (!strUser.IsEmpty() && strUser.CompareNoCase (theApp.m_strAllUser) != 0)
	{
		strFlags+= _T("-i");
		strFlags+= strUser;
		strFlags+= _T(" ");
	}
	if (pQueryInfo->GetAfterCheckPoint())  /* if the GUI allows checkpoint# 0, let the corresponding error message appear */
	{
		CString strCheckPoint;
		strCheckPoint.Format (_T("#c%d "), pQueryInfo->GetCheckPointNumber());
		strFlags += strCheckPoint;
	}
	CString strBefore;
	CString strAfter;
	pQueryInfo->GetBeforeAfter (strBefore, strAfter);
	if (!strBefore.IsEmpty())
	{
		strFlags+= _T("-e");
		strFlags+= strBefore;
		strFlags+= _T(" ");
	}
	if (!strAfter.IsEmpty())
	{
		strFlags+= _T("-b");
		strFlags+= strAfter;
		strFlags+= _T(" ");
	}
	
	strSyntax.Format (
		_T("auditdb %s -u%s -internal_data -aborted_transactions -no_header -limit_output -expand_lobjs %s"),
		(LPCTSTR)strDatabase,
		(LPCTSTR)ConnectedUser,
		(LPCTSTR)strFlags);

	return;
}


BOOL IJA_llQueryDatabaseTransaction (
    CaQueryTransactionInfo* pQueryInfo, 
    CTypedPtrList < CObList, CaDatabaseTransactionItemData* >& listTransaction)
{
	CString strRemoteOutput;
	//
	// This function has shown error already in case of error:
	if (!IJA_DatabaseAuditdbOutput (pQueryInfo, strRemoteOutput))
		return FALSE;

	if (!IJA_DatabaseParse4Transactions (strRemoteOutput, listTransaction))
	{
		//
		// Error while getting the journaled transactions list.
		CString strMsg;
		strMsg.LoadString(IDS_FAILED_TO_GET_JOURNALED_TRANS);

		AfxMessageBox (strMsg);
		return FALSE;
	}

	return TRUE;
}



static BOOL DropTempTable(CaSession* pSession, LPCTSTR lpTable, BOOL bLocalAndNoRmcmd)
{
	if (!bLocalAndNoRmcmd) {
		CaTemporarySession session(pSession->GetNode(), pSession->GetDatabase());
		if (!session.IsConnected())
			return FALSE;
		CString Statement;
		Statement.Format(_T("drop table %s"), lpTable);
		CaLowlevelAddAlterDrop param (_T(""), _T(""), Statement);
		param.NeedSession(FALSE);
		if (!param.ExecuteStatement(NULL))
			return FALSE;
		session.Release();
	}
	return TRUE;
}

BOOL IJA_llQueryTableTransaction (
    CaQueryTransactionInfo* pQueryInfo,
    CTypedPtrList<CObList, CaColumn*>* pListColumn,
    CTypedPtrList < CObList, CaTableTransactionItemData* >& listTransaction)
{
	CString strDatabase;
	CString strDatabaseOwner;
	CString strTable;
	CString strTableOwner;
	CString strStatement;

	pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);
	pQueryInfo->GetTable (strTable, strTableOwner);

	//
	// Open the session:
	CaTemporarySession session (pQueryInfo->GetNode(), strDatabase);
	if (!session.IsConnected())
	{
		//
		// Failedto get Session.
		CString strMsg;
		strMsg.LoadString (IDS_FAIL_TO_GETSESSION);
		AfxMessageBox (strMsg);
		return FALSE;
	}

	CString strTempTable = _T("");
	CString csGranteeList = _T(""); /* no grantee list required here */
	BOOL bOK = IJA_TableAuditdbOutput (pQueryInfo, &session, strTempTable, pListColumn, csGranteeList);
	if (!bOK) {
		session.Release(SESSION_ROLLBACK);
		return FALSE;
	}
	BOOL bOnLocal = session.IsLocalNode();
	//
	// STEP 1: Select row that are INSERT, DELETE, REPOLD, REPNEW
	// Select the rows (table transactions) from the newly created Table:
	CString strSessionPrefix = _T("");
	if (bOnLocal)
		strSessionPrefix = _T("session.");

	strStatement.Format (
		_T("select * from %s%s where operation  in ('repold', 'repnew', 'append', 'insert', 'delete')"),
		(LPCTSTR)strSessionPrefix,
		(LPCTSTR)strTempTable);
	CaIjaCursor cursor(1, strStatement);
	if (cursor.Open())
	{
		int nCount = 0;
		CStringList listData;
		CString strItem1;
		CString strItem2;

		while (cursor.Fetch(listData, nCount))
		{
			CaTableTransactionItemData* pTran = new CaTableTransactionItemData();

			POSITION pos = listData.GetHeadPosition();
			ASSERT (listData.GetCount() >= 10); // At least 10 columns;
			if (listData.GetCount() < 10)
			{
				delete pTran;
				return FALSE;
			}
			pTran->SetTable (strTable);
			pTran->SetTableOwner (strTableOwner);

			if (pos != NULL)
			{
				// LSN:
				strItem1 = listData.GetNext (pos);
				strItem2 = listData.GetNext (pos);
				pTran->SetLsn (_ttoul(strItem1), _ttoul(strItem2));
				// TID:
				strItem1 = listData.GetNext (pos); 
				pTran->SetTid(_ttol(strItem1));
				// Date:
				strItem1 = listData.GetNext (pos);
				pTran->SetDate(strItem1);
				// User Name:
				strItem1 = listData.GetNext (pos);
				pTran->SetUser (strItem1);
				// Operation:
				strItem1 = listData.GetNext (pos);
				if (strItem1.CompareNoCase (_T("insert")) == 0 || strItem1.CompareNoCase (_T("append")) == 0)
					pTran->SetOperation (T_INSERT);
				else
				if (strItem1.CompareNoCase (_T("delete")) == 0)
					pTran->SetOperation (T_DELETE);
				else
				if (strItem1.CompareNoCase (_T("repold")) == 0)
					pTran->SetOperation (T_BEFOREUPDATE);
				else
				if (strItem1.CompareNoCase (_T("repnew")) == 0)
					pTran->SetOperation (T_AFTERUPDATE);
				else
				{
					ASSERT (FALSE);
					pTran->SetOperation (T_UNKNOWN);
				}
				// Transaction:
				strItem1 = listData.GetNext (pos);
				strItem2 = listData.GetNext (pos);
				pTran->SetTransactionID (_ttoul(strItem1), _ttoul(strItem2));
				// Ignore:
				strItem1 = listData.GetNext (pos);
				strItem2 = listData.GetNext (pos);
			}
			pTran->SetCommit  (TRUE);
			pTran->SetJournal (TRUE);

			CStringList& listAuditedData = pTran->GetListData();
			while (pos != NULL)
			{
				strItem1 = listData.GetNext (pos);
				listAuditedData.AddTail(strItem1);
			}

			listTransaction.AddTail (pTran);
			listData.RemoveAll();
		}

		cursor.Close();
	}

	//
	// STEP 2: Select rows that are COMMIT, ABORT, COMMITNJ, ABORTNJ
	//         to update the previous select result if the rows are commit, abort, ...
	// Select the rows (table transactions) from the newly created Table:
	strStatement.Format (
		_T("select * from %s%s where operation  in ('commit', 'abort', 'commitnj', 'abortnj')"),
		(LPCTSTR)strSessionPrefix,
		(LPCTSTR)strTempTable);

	CaIjaCursor cursor2(2, strStatement);
	if (cursor2.Open())
	{
		CaTableTransactionItemData* pFound = NULL;
		int nCount = 0;
		CStringList listData;
		CString strItem1;
		CString strItem2;
		CString strOperation;

		while (cursor2.Fetch(listData, nCount))
		{
			POSITION pos = listData.GetHeadPosition();
			ASSERT (listData.GetCount() >= 10); // At least 10 columns;
			if (listData.GetCount() < 10)
				return FALSE;

			if (pos != NULL)
			{
				// LSN:
				strItem1 = listData.GetNext (pos);
				strItem2 = listData.GetNext (pos);
				// Ignore:
				strItem1 = listData.GetNext (pos); 
				// Date:
				strItem1 = listData.GetNext (pos);
				// User Name:
				strItem1 = listData.GetNext (pos);
				// Operation:
				strOperation = listData.GetNext (pos);

				// Transaction:
				strItem1 = listData.GetNext (pos);
				strItem2 = listData.GetNext (pos);
				TRANSACTION_ChangeState (_ttoul(strItem1), _ttoul(strItem2), strOperation, listTransaction);
				// Ignore:
				strItem1 = listData.GetNext (pos);
				strItem2 = listData.GetNext (pos);
			}
			listData.RemoveAll();
		}

		cursor2.Close();
	}

	//
	// Make sure that the temporary table has been destroyed:
	if (!bOnLocal && bOK && !strTempTable.IsEmpty()) 
	{
		strStatement.Format ( _T("drop table %s"), (LPCTSTR)strTempTable);
		CaLowlevelAddAlterDrop param (pQueryInfo->GetNode(), strDatabase, strStatement);
		param.NeedSession(FALSE); // Use the current open session.
		param.SetStatement(strStatement);
		if (!param.ExecuteStatement(NULL))
		{
			CString strMsg;
			strMsg.LoadString(IDS_FAIL_DROP_TEMP_TABLE);
			AfxMessageBox(strMsg);
		}
	}

	session.Release(); // Release session and commit.
	return TRUE;
}



BOOL IJA_llQueryDatabaseTransactionDetail (
    CaQueryTransactionInfo* pQueryInfo, 
    CaDatabaseTransactionItemData* pDatabaseTransaction,
    CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction)
{
	//
	// TODO:
	ASSERT (FALSE);

	return TRUE;
}

BOOL IJA_llQueryTableTransactionData (
    CaQueryTransactionInfo* pQueryInfo,
    CaTableTransactionItemData* pTableTrans)
{
	CString strStatement;
	CString strDatabase;
	CString strDatabaseOwner;
	CString strTable;
	CString strTableOwner;
	if (!pQueryInfo)
		return FALSE;
	pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);
	pQueryInfo->GetTable (strTable, strTableOwner);

	CaSessionManager& smgr = theApp.GetSessionManager();
	CaSession session;
	session.SetNode(pQueryInfo->GetNode());
	session.SetDatabase(strDatabase);
	session.SetIndependent(TRUE);
	CaSessionUsage sessionUsage(&smgr, &session, SESSION_COMMIT); // commit on release

	//
	// TODO: Replace this statement to select from the temporary table:
	strStatement.Format (_T("select * from %s.%s"), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
	CaIjaCursor* pCursor = new CaIjaCursor (1, strStatement);
	if (pCursor && pCursor->Open())
	{
		int nCount = 0;
		CStringList& listData = pTableTrans->GetListData();
		//
		// Fetch only one row for a transaction <pTableTrans>
		pCursor->Fetch(listData, nCount);
		pCursor->Close();
	}

	return TRUE;
}


BOOL PROCESS_Execute(LPCTSTR lpszCommandLine, CString& strError, BOOL bOutputAlway)
{
	BOOL bSuccess = FALSE;
	HANDLE hChildStdoutRd, hChildStdoutWr;
	HANDLE hChildStdinRd, hChildStdinWr;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	DWORD dwLastError = 0;
	CString strFail  = _T("Execute process failed !");
	int nStatus = 0;
	try
	{
		strError = _T("");
#if defined (SIMULATION)
		return TRUE;
#endif
		//
		// Init security attributes
		SECURITY_ATTRIBUTES sa;
		memset (&sa, 0, sizeof (sa));
		sa.nLength              = sizeof(sa);
		sa.bInheritHandle       = TRUE; // make pipe handles inheritable
		sa.lpSecurityDescriptor = NULL;

		//
		// Create a pipe for the child's STDOUT
		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);
			strFail = (LPCTSTR)lpMsgBuf;
			LocalFree(lpMsgBuf);
			throw (LPCTSTR)strFail;
		}
		//
		// Create a pipe for the child's STDIN
		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);
			strFail = (LPCTSTR)lpMsgBuf;
			LocalFree(lpMsgBuf);
			throw (LPCTSTR)strFail;
		}
		
		//
		// Duplicate the write handle to the STDIN pipe so it's not inherited
		if (!DuplicateHandle(
			 GetCurrentProcess(), 
			 hChildStdinWr, 
			 GetCurrentProcess(), 
			 &hChildStdinWr, 
			 0, FALSE, 
			 DUPLICATE_SAME_ACCESS)) 
		{
			dwLastError = GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);
			strFail = (LPCTSTR)lpMsgBuf;
			LocalFree(lpMsgBuf);
			throw (LPCTSTR)strFail;
		}
		//
		// Don't need it any more
		CloseHandle(hChildStdinWr);

		//
		// Create the child process
		StartupInfo.cb = sizeof(STARTUPINFO);
		StartupInfo.lpReserved = NULL;
		StartupInfo.lpReserved2 = NULL;
		StartupInfo.cbReserved2 = 0;
		StartupInfo.lpDesktop = NULL;
		StartupInfo.lpTitle = NULL;
		StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		StartupInfo.wShowWindow = SW_HIDE;
		StartupInfo.hStdInput = hChildStdinRd;
		StartupInfo.hStdOutput = hChildStdoutWr;
		StartupInfo.hStdError = hChildStdoutWr;

		bSuccess = CreateProcess(
			NULL,                    // pointer to name of executable module
			(LPTSTR)lpszCommandLine, // pointer to command line string
			NULL,                    // pointer to process security attributes
			NULL,                    // pointer to thread security attributes
			TRUE ,                   // handle inheritance flag
			CREATE_NEW_CONSOLE ,        // creation flags
			NULL,                    // pointer to new environment block
			NULL,                    // pointer to current directory name
			&StartupInfo,            // pointer to STARTUPINFO
			&ProcInfo                // pointer to PROCESS_INFORMATION
		);
		CloseHandle(hChildStdinRd);
		CloseHandle(hChildStdoutWr);

		if (!bSuccess)
		{
			dwLastError = GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage (
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dwLastError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);
			strError  = _T("PROCESS_Execute: execute process failed.\n");
			strError += (LPCTSTR)lpMsgBuf;
			LocalFree(lpMsgBuf);
			return FALSE;
		}

		//
		// Reading data from the pipe:
		int nTried = 0;
		const int nSize = 128;
		TCHAR tchszBuffer [nSize];
		DWORD dwBytesRead;
		while (ReadFile(hChildStdoutRd, tchszBuffer, nSize-2, &dwBytesRead, NULL))
		{
			if (dwBytesRead > 0)
			{
				tchszBuffer [(int)dwBytesRead] = _T('\0');
				strError+= tchszBuffer;
				tchszBuffer[0] = _T('\0');
			}
			else
			{
				//
				// The file pointer was beyond the current end of the file
				DWORD dwExitCode = 0;
				if (!GetExitCodeProcess(ProcInfo.hProcess, &dwExitCode)) 
				{
					strError  = _T("PROCESS_Execute: execute process failed.\n");
					return FALSE;
				}

				if (dwExitCode != STILL_ACTIVE)
					break;
				else
				{
					Sleep (1000);
					nTried++;
					if (nTried > 2*60)
					{
						strError  = _T("PROCESS_Execute: execute process failed.\n");
						return FALSE;
					}
				}
			}
		}
		if (!bOutputAlway && !strError.IsEmpty())
			return FALSE;
	}
	catch (LPCTSTR lpszStr)
	{
		strError = _T("PROCESS_Execute: execute process failed.\n");
		strError += lpszStr;
		return FALSE;
	}
	catch (...)
	{
		bSuccess=FALSE;
		TRACE0 ("PROCESS_Execute \n");
	}

	return bSuccess;
}

static BOOL DATABASE_AuditdbOutput(LPCTSTR lpszAuditDb, CString& strOutput)
{
	//
	// Use the PROCESS_Execute(LPCTSTR lpszCommandLine, CString& strError, BOOL bOutputAlway = FALSE),
	// if the function returns TRUE, then the strError will be the output to be parsed.
	// otherwise the strError is the error text ...
	BOOL bSuccess = PROCESS_Execute(lpszAuditDb, strOutput, TRUE);
	return bSuccess;
}

//
// DATABASE LEVEL output of auditdb command.
// It might throw the exception CeSqlException if fail in querying something from Ingres Database:
BOOL IJA_DatabaseAuditdbOutput (CaQueryTransactionInfo* pQueryInfo, CString& strOutput)
{
	CString strDatabase;
	CString strDatabaseOwner;
	CString strStatement;

	pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);
	CString strConnection;
	strConnection.Format (_T("%s::%s"), (LPCTSTR)pQueryInfo->GetNode(), (LPCTSTR)strDatabase);
	GenerateDBLevelAuditdbSyntax(pQueryInfo, strStatement, (LPCTSTR)pQueryInfo->GetConnectedUser());
	BOOL bLocalNode = (strConnection.Find(_T("(local)"))==0);
	BOOL bResult = FALSE;
	if (bLocalNode)
	{
		bResult = DATABASE_AuditdbOutput(strStatement, strOutput);
		if (!bResult)
		{
			AfxMessageBox (strOutput, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
	}
	else
	{
		bResult = ExecRmcmdTestOutPut ((LPTSTR)(LPCTSTR)pQueryInfo->GetNode(), (LPTSTR)(LPCTSTR)strStatement,CMD_DBLEVEL_AUDITDB, strOutput); 
		if (!bResult) 
		{
			// 
			// Error While executing the AuditDB Command:\n\n%1
			CString strMsg;
			AfxFormatString1 (strMsg, IDS_FAIL_TO_EXECUTE_AUDITDB, (LPCTSTR)strOutput);

			AfxMessageBox (strMsg);
			return FALSE;
		}
	}
	return TRUE;
}


//
// TABLE LEVEL output of auditdb command. 
// ***Only one table and one file in the auditdb arguments can be specified***
//
// It might throw the exception CeSqlException if fail in querying something from Ingres Database.
// If you have the list of columns, then provide it !.
// If the list of columns is null, then the function will query the list of columns !
// strTempTable [OUT]: receives the output of the temporary table name
BOOL IJA_TableAuditdbOutput (
    CaQueryTransactionInfo* pQueryInfo,
    CaTemporarySession* pSession, 
    CString& strTempTable, 
    CTypedPtrList<CObList, CaColumn*>* pListColumn,
	CString& strGranteeList)
{
	CString strDatabase;
	CString strDatabaseOwner;
	CString strTable;
	CString strTableOwner;
	CString strStatement;
	CString strLocalIngresTemp = theApp.m_strLocalIITemporary;
	if (strLocalIngresTemp.IsEmpty())
		return FALSE;
#ifdef MAINWIN
	strLocalIngresTemp += _T("/");
#else
	strLocalIngresTemp += _T("\\");
#endif

	class CaLocalData
	{
	public:
		CaLocalData(BOOL bLocal):m_bLocal(bLocal), m_strFile(_T("")){}
		virtual ~CaLocalData()
		{
			while (!m_listObject.IsEmpty())
				delete m_listObject.RemoveHead();
			//
			// Prepare to delete the files (*.trl) outputed from AUDITDB:
			if (m_bLocal)
			{
				if (!m_strFile.IsEmpty() && (_taccess(m_strFile, 0) != -1))
					DeleteFile (m_strFile);
			}
		};
		
		BOOL m_bLocal;
		CString m_strFile;
		CTypedPtrList<CObList, CaColumn*> m_listObject;
	};
	//
	// On the local node, do not use the remote command (RMCMD)
	BOOL bOnLocal = pSession->IsLocalNode();
	
	CaLocalData localData(bOnLocal);
	BOOL bQueryColumns = pListColumn? FALSE: TRUE;
	if (bQueryColumns)
	{
		if (IJA_QueryColumn (pQueryInfo, localData.m_listObject))
			pListColumn = &(localData.m_listObject);
	}

	CString csRmcmdOutPut;
	BOOL bResult = TRUE;

	ASSERT (pSession);
	if (!pSession)
		return FALSE;

	pQueryInfo->GetTable (strTable, strTableOwner);
	//
	// Get temp table name
	if (!GetTempAuditTableName(strTempTable, (LPCTSTR)strTable, (LPCTSTR)pQueryInfo->GetConnectedUser()))
	{
		//
		// Error while generating a temporary file name.
		CString strMsg;
		strMsg.LoadString(IDS_FAIL_TO_GENERATE_TEMPFILE);
		
		AfxMessageBox (strMsg);
		return FALSE;
	}
	
	//
	// Generate & Create the table to contain the audited table rows:
	// -------------------------------------------------------------
	CString strSyntaxCreateTable;
	BOOL bCreate = RCRD_GenerateTemporaryTableSyntax((LPCTSTR)strTempTable, pListColumn, strSyntaxCreateTable, bOnLocal);
	if (!bCreate) 
	{
		CString strMsg;
		strMsg.LoadString(IDS_FAIL_TO_GENERATE_TEMPTABLE);

		AfxMessageBox (strMsg);
		return FALSE;
	}
	CaLowlevelAddAlterDrop param (pQueryInfo->GetNode(), strDatabase, strSyntaxCreateTable);

	//
	// Create the temporary table in the current session:
	// --------------------------------------------------
	param.NeedSession(FALSE);
	//
	// This function will throw exception if failed:
	bCreate = param.ExecuteStatement(NULL);

	if (!bOnLocal)
	{
		param.SetStatement(_T("commit"));
		//
		// This function will throw exception if failed:
		bCreate = param.ExecuteStatement(NULL);
		if (!strGranteeList.IsEmpty()) {
			CString csStatement;
			csStatement.Format("grant select on %s to %s",(LPCTSTR)strTempTable, (LPCTSTR) strGranteeList);
			param.SetStatement((LPCTSTR)csStatement);
			// This function will throw exception in case of failure
			bCreate = param.ExecuteStatement(NULL);
			param.SetStatement(_T("commit"));
			// This function will throw exception in case of failure
			bCreate = param.ExecuteStatement(NULL);
		}
	}

	//
	// Construct parameters for the need of generating AUDITDB Syntax:
	CString strFile;
	CStringList listFile;
	CTypedPtrList < CObList, CaIjaTable* > listTable;
	
	//
	// TODO: Construct the directory path on the local machine:
	//       On remote machine the file name is generate by the utility adbtofst on NT machine
	//       or used the pipe on unix machine see adbtofst 
	if (!bOnLocal)
	{
		strFile = _T("");
	}
	else
	{

		CString strAcceptedFileName = strTempTable;
		MakeFileName(strAcceptedFileName);

		strFile.Format (_T("%s%s.trl"), (LPCTSTR)strLocalIngresTemp, (LPCTSTR)strAcceptedFileName);
	}

	listFile.AddTail (strFile);
	listTable.AddTail (new CaIjaTable(strTable, strTableOwner));
	localData.m_strFile = strFile;
	//
	// Generate AUDITDB syntax:
	BOOL bSyntax = FALSE; // FALSE generate syntax for the standard auditdb command
	                      // TRUE  generate syntax for adbtofst
	if (!bOnLocal)
		bSyntax = TRUE;

	RCRD_GenerateAuditdbSyntax (pQueryInfo, listTable, listFile, strStatement, pQueryInfo->GetConnectedUser(),bSyntax);
	while (!listTable.IsEmpty())
		delete listTable.RemoveHead();

	if (bOnLocal) 
	{
		CString strError = _T("");
		//
		// Execute the AUDITDB command:
		bCreate = PROCESS_Execute (strStatement, strError);
		if (!bCreate) 
		{
			if (!strError.IsEmpty())
				AfxMessageBox (strError);
			return FALSE;
		}
		//
		// Check if the file has been successfuly created:
		if (_taccess(strFile, 0) == -1)
		{
			//
			// auditdb ...-file=<file name>, has failed to create file.
			CString strMsg;
			strMsg.LoadString(IDS_AUDITDB_FAILS_TO_CREATE_FILE);

			AfxMessageBox (strMsg);
			return FALSE;
		}
		//
		// Copy the binary data of audited table to the newly created Table:
		strStatement.Format (
			_T("copy table session.%s () from '%s'"), 
			(LPCTSTR)strTempTable,
			(LPCTSTR)strFile);
		param.SetStatement(strStatement);

		//
		// This function will throw exception if failed:
		param.ExecuteStatement(NULL);
		param.SetStatement(_T("commit")); // the table is temporary anyhow. need to commit because of potential "set session authorization" statements
		bCreate = param.ExecuteStatement(NULL);
	}
	else
	//
	// Execute ADBTOFST at the remote machine through RMCMD:
	{
		//
		// At this point, it requires that no sessions are opened !
		pSession->Release();
		
		//Generate adbtofst syntax
		CString AdbToFstSyntax;
		CString strDatabase;
		CString strDatabaseOwner;

		pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);

		AdbToFstSyntax.Format(_T("adbtofst -u%s %s %s %s"),
		    (LPCTSTR)pQueryInfo->GetConnectedUser(),
		    (LPCTSTR)strTempTable,(LPCTSTR)strDatabase,
		    (LPCTSTR)strStatement);
		//
		// Execute the ADBTOFST command on the remote machine:
		bCreate = ExecRmcmdTestOutPut ((LPTSTR)(LPCTSTR)pQueryInfo->GetNode(), (LPTSTR)(LPCTSTR)AdbToFstSyntax,CMD_FASTLOAD,csRmcmdOutPut); 
		if (!bCreate) {
			//
			// Error While executing the AdbToFst Command:\n\n%1
			CString strMsg;
			if (csRmcmdOutPut.Find("E_DM1601_FLOAD_OPEN_DB")>=0)
				AfxFormatString1(strMsg, IDS_FAIL_TO_EXECUTE_ADBTOFST, (LPCTSTR)csRmcmdOutPut);
			else 
				strMsg = csRmcmdOutPut;
			AfxMessageBox (strMsg);
			DropTempTable(pSession, (LPCTSTR)strTempTable, bOnLocal);
			return FALSE;
		}

		//
		// Reopen the previous session:
		pSession->Open();
		if (!pSession->IsConnected())
		{
			//
			// Failedto get Session.
			CString strMsg;
			strMsg.LoadString (IDS_FAIL_TO_GETSESSION);
			AfxMessageBox (strMsg);

			DropTempTable(pSession, (LPCTSTR)strTempTable, bOnLocal);
			return FALSE;
		}
		param.NeedSession(FALSE);
	}

	return TRUE;
}



