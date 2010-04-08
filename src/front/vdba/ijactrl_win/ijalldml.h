/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : ijalldml.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01),
**
**    Purpose  : Low level Data Manipulation
**
** History:
** 22-Dec-1999 (uk$so01)
**    Created
** 14-Jun-2001 (noifr01)
**    (bug 104954) changed a proptotype for managing the list of grantees
**    on the temporary tables
**
**/

#if !defined(IJALLDML_HEADER)
#define IJALLDML_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "logadata.h"

BOOL IJA_llQueryDatabaseTransaction (
    CaQueryTransactionInfo* pQueryInfo, 
    CTypedPtrList < CObList, CaDatabaseTransactionItemData* >& listTransaction);

BOOL IJA_llQueryTableTransaction (
    CaQueryTransactionInfo* pQueryInfo, 
    CTypedPtrList<CObList, CaColumn*>* pListColumn,
    CTypedPtrList < CObList, CaTableTransactionItemData* >& listTransaction);

BOOL IJA_llQueryDatabaseTransactionDetail (
    CaQueryTransactionInfo* pQueryInfo, 
    CaDatabaseTransactionItemData* pDatabaseTransaction,
    CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction);

BOOL IJA_llQueryTableTransactionData (
    CaQueryTransactionInfo* pQueryInfo,
    CaTableTransactionItemData* pTableTrans);

BOOL PROCESS_Execute(LPCTSTR lpszCommandLine, CString& strError, BOOL bOutputAlway = FALSE);
//
// DATABASE LEVEL output of auditdb command.
// It might throw the exception CeSqlException if fail in querying something from Ingres Database:
BOOL IJA_DatabaseAuditdbOutput (CaQueryTransactionInfo* pQueryInfo, CString& strOutput);

//
// TABLE LEVEL output of auditdb command. 
// ***Only one table and one file in the auditdb arguments can be specified***
//
// It might throw the exception CeSqlException if fail in querying something from Ingres Database.
// If you have the list of columns, then provide it !.
// If the list of columns is null, then the function will query the list of columns !
class CaTemporarySession;
BOOL IJA_TableAuditdbOutput (
    CaQueryTransactionInfo* pQueryInfo,
    CaTemporarySession* pSession, 
    CString& strTempTable, 
    CTypedPtrList<CObList, CaColumn*>* pListColumn,
	CString& strGranteeList);

#endif // IJALLDML_HEADER