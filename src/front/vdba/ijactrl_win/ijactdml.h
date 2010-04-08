/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijactdml.h , Header File
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
**/

#if !defined(IJACTDML_HEADER)
#define IJACTDML_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "logadata.h"


BOOL IJA_QueryDatabaseTransaction (
    CaQueryTransactionInfo* pQueryInfo, 
    CTypedPtrList < CObList, CaDatabaseTransactionItemData* >& listTransaction);

BOOL IJA_QueryTableTransaction (
    CaQueryTransactionInfo* pQueryInfo, 
    CTypedPtrList<CObList, CaColumn*>* pListColumn,
    CTypedPtrList < CObList, CaTableTransactionItemData* >& listTransaction);

BOOL IJA_QueryDatabaseTransactionDetail (
    CaQueryTransactionInfo* pQueryInfo, 
    CaDatabaseTransactionItemData* pDatabaseTransaction,
    CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction);

BOOL IJA_QueryTableTransactionData (
    CaQueryTransactionInfo* pQueryInfo,
    CaTableTransactionItemData* pTableTrans);

BOOL IJA_QueryDatabase (LPCTSTR lpszNode, CTypedPtrList<CObList, CaDatabase*>& listDatabase);
BOOL IJA_QueryColumn   (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaColumn*>& listObject, BOOL bNeedSession = TRUE);
BOOL IJA_QueryRule     (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaDBObject*>& listObject, CaSession* pSession);
BOOL IJA_QueryIntegrity(CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaDBObject*>& listObject, CaSession* pSession);
BOOL IJA_QueryConstraint   (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaDBObject*>& listObject, CaSession* pSession);
BOOL IJA_QueryUserInCurSession (CString& strUserName);

BOOL IJA_GetArchiverInternal ();
void IJA_ActivateLog2Journal ();
void IJA_LogFileStatus(CString& csCurStatus);

#endif // IJACTDML_HEADER