/*
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijactdml.cpp , Implementation File
**    Project  : Ingres Journal Analyser 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data Manipulation
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    created
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 02-Feb-2004 (noifr01)
**    removed test code that was causing problem with new versions of the
**    compiler (with iostream libraries)
**/


#include "stdafx.h"
#include "ijactdml.h"
#include "ijalldml.h"
#include "ijadmlll.h"
#include "ingobdml.h"
#include "dmlinteg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//
// not published fron libingll.lib
extern BOOL INGRESII_llDBProcedureOfRule (CaLLQueryInfo* pQueryInfo, CaDBObject* pRule, CaProcedure& procedure);




BOOL IJA_QueryDatabaseTransaction (
    CaQueryTransactionInfo* pQueryInfo, 
    CTypedPtrList < CObList, CaDatabaseTransactionItemData* >& listTransaction)
{
	try
	{
		return IJA_llQueryDatabaseTransaction (pQueryInfo, listTransaction);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error, IJA_QueryDatabaseTransaction, failed to query transactions"));
		return FALSE;
	}

	return TRUE;
}


BOOL IJA_QueryTableTransaction (
    CaQueryTransactionInfo* pQueryInfo, 
    CTypedPtrList<CObList, CaColumn*>* pListColumn,
    CTypedPtrList < CObList, CaTableTransactionItemData* >& listTransaction)
{
	BOOL bOk = FALSE;
	try
	{
		bOk =IJA_llQueryTableTransaction (pQueryInfo, pListColumn, listTransaction);
	}

	catch (CeSqlException e)
	{
		AfxMessageBox(e.GetReason());
		bOk = FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error, IJA_QueryTableTransaction, failed to query transactions"));
		bOk = FALSE;
	}
	if (!bOk)
	{
		while (!listTransaction.IsEmpty())
			delete listTransaction.RemoveHead();
	}

	return bOk;
}

BOOL IJA_QueryDatabaseTransactionDetail (
    CaQueryTransactionInfo* pQueryInfo, 
    CaDatabaseTransactionItemData* pDatabaseTransaction,
    CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction)
{
	try
	{
		return IJA_llQueryDatabaseTransactionDetail(pQueryInfo, pDatabaseTransaction, listTransaction);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error, IJA_QueryDatabaseTransactionDetail, failed to query transactions"));
		return FALSE;
	}

	return TRUE;
}


BOOL IJA_QueryTableTransactionData (
    CaQueryTransactionInfo* pQueryInfo,
    CaTableTransactionItemData* pTableTrans)
{
	try
	{
#if defined (SIMULATION)
		CStringList& listData = pTableTrans->GetListData();
		listData.AddTail(_T("\"AAAA\""));
		listData.AddTail(_T("\"BBBB\""));
		listData.AddTail(_T("\"CCCC\""));
#else
		return IJA_llQueryTableTransactionData(pQueryInfo, pTableTrans);
#endif
	}
	catch (...)
	{
		AfxMessageBox (_T("System error, IJA_QueryTableTransactionData, failed to query transactions data"));
		return FALSE;
	}

	return TRUE;
}

BOOL IJA_QueryDatabase (LPCTSTR lpszNode, CTypedPtrList<CObList, CaDatabase*>& listDatabase)
{
	CaLLQueryInfo info(OBT_DATABASE, lpszNode);
	CTypedPtrList<CObList, CaDBObject*> listObject;
	info.SetIndependent(TRUE);
	INGRESII_llQueryObject (&info, listObject, (void*)(&theApp.GetSessionManager()));
	POSITION pos = listObject.GetHeadPosition();
	while (pos != NULL)
	{
		CaDatabase* pDatabase = (CaDatabase*)listObject.GetNext(pos);
		listDatabase.AddTail(pDatabase);
	}
	return TRUE;
}

BOOL IJA_QueryColumn (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaColumn*>& listObject, BOOL bNeedSession)
{
	CString strDatabase, strDatabaseOwner;
	CString strTable, strTableOwner;
	pQueryInfo->GetTable (strTable, strTableOwner);
	pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);
	CaLLQueryInfo info(OBT_TABLECOLUMN, pQueryInfo->GetNode(), strDatabase);
	info.SetItem2(strTable, strTableOwner);
	if (pQueryInfo->GetUser().CompareNoCase(theApp.m_strAllUser) != 0)
		info.SetUser(pQueryInfo->GetUser());
	CTypedPtrList< CObList, CaDBObject* > ls;
	if (bNeedSession)
	{
		info.SetIndependent(TRUE);
		INGRESII_llQueryObject (&info, ls, (void*)(&theApp.GetSessionManager()));
	}
	else
	{
		CaSession session;
		session.SetNode(pQueryInfo->GetNode());
		session.SetDatabase(strDatabase);
		if (pQueryInfo->GetUser().CompareNoCase(theApp.m_strAllUser) != 0)
			session.SetUser(pQueryInfo->GetUser());
		INGRESII_llQueryObject2 (&info, ls, &session);
	}
	POSITION pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = (CaColumn*)ls.GetNext(pos);
		listObject.AddTail(pCol);
	}
	return TRUE;
}


BOOL IJA_QueryRule   (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaDBObject*>& listObject, CaSession* pSession)
{

	//
	// In the following code, the 
	//   - INGRESII_llQueryObject2
	//   - INGRESII_llQueryDetailObject2
	// are used because the caller of IJA_QueryRule, the RCRD_TablRuleConstraint has already opened
	// a temporary session:

	CString strDatabase, strDatabaseOwner;
	CString strTable, strTableOwner;
	pQueryInfo->GetTable (strTable, strTableOwner);
	pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);
	CaLLQueryInfo info(OBT_RULE, pQueryInfo->GetNode(), strDatabase);
	info.SetItem2(strTable, strTableOwner);
	if (pQueryInfo->GetUser().CompareNoCase(theApp.m_strAllUser) != 0)
		info.SetUser(pQueryInfo->GetUser());

	CTypedPtrList<CObList, CaDBObject*> ls;
	INGRESII_llQueryObject2 (&info, ls, pSession);
	//
	// For each rule, get the detail of its procedure:
	info.SetObjectType(OBT_PROCEDURE);
	POSITION pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		CaRule* pItem = (CaRule*)ls.GetNext (pos);
		CaIjaRule* pRule = new CaIjaRule(pItem->GetName(), pItem->GetOwner());
		pRule->SetBaseTable(strTable, strTableOwner);
		listObject.AddTail(pRule);

		info.SetObjectType(OBT_RULE);
		INGRESII_llQueryDetailObject2 (&info, (CaDBObject*)pRule, pSession);
		delete pItem;

		info.SetObjectType(OBT_PROCEDURE);
		if (INGRESII_llDBProcedureOfRule (&info, pRule, pRule->m_procedure))
		{
			INGRESII_llQueryDetailObject2 (&info, (CaDBObject*)&(pRule->m_procedure), pSession);
		}
	}

	return TRUE;
}

BOOL IJA_QueryIntegrity (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaDBObject*>& listObject, CaSession* pSession)
{
	CString strDatabase, strDatabaseOwner;
	CString strTable, strTableOwner;
	pQueryInfo->GetTable (strTable, strTableOwner);
	pQueryInfo->GetDatabase (strDatabase, strDatabaseOwner);

	CaLLQueryInfo info(OBT_INTEGRITY, pQueryInfo->GetNode(), strDatabase);
	info.SetItem2(strTable, strTableOwner);
	if (pQueryInfo->GetUser().CompareNoCase(theApp.m_strAllUser) != 0)
		info.SetUser(pQueryInfo->GetUser());

	//
	// In the following code, the 
	//   - INGRESII_llQueryObject2
	//   - INGRESII_llQueryDetailObject2
	// are used because the caller of IJA_QueryIntegrity, the RCRD_TablRuleConstraint has already opened
	// a temporary session:

	CTypedPtrList<CObList, CaDBObject*> ls;
	INGRESII_llQueryObject2 (&info, ls, pSession);
	POSITION pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		CaIntegrity* pItem = (CaIntegrity*)ls.GetNext (pos);
		CaIntegrityDetail* pDetail = new CaIntegrityDetail(pItem->GetName(), pItem->GetOwner());
		pDetail->SetBaseTable(strTable, strTableOwner);
		pDetail->SetNumber(pItem->GetNumber());
		listObject.AddTail(pDetail);
		delete pItem;

		INGRESII_llQueryDetailObject2 (&info, pDetail, pSession);
	}
	return TRUE;
}


BOOL IJA_QueryConstraint (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaDBObject*>& listObject, CaSession* pSession)
{
	return INGRESII_llQueryConstraint (pQueryInfo, listObject);
}

BOOL IJA_QueryUserInCurSession (CString& strUserName)
{
#if defined (SIMULATION)
	strUserName = _T("Simulated Connected User");
	return TRUE;
#else
	return IJA_llQueryUserInCurSession (strUserName);
#endif
}

BOOL IJA_GetArchiverInternal()
{
#if defined (SIMULATION)
	ASSERT (FALSE); // Not implemented
#else
	return IJA_llGetArchiverInternal();
#endif
}

void IJA_ActivateLog2Journal()
{
#if defined (SIMULATION)
	ASSERT (FALSE); // Not implemented
#else
	IJA_llActivateLog2Journal();
#endif
}

void IJA_LogFileStatus(CString& csCurStatus)
{
#if defined (SIMULATION)
	ASSERT (FALSE); // Not implemented
#else
	IJA_llLogFileStatus(csCurStatus);
#endif
}
