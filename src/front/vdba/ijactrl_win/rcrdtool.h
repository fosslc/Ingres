/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : rcrdtool.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Common tools for dialog "Recover" and "Redo"
**
** History:
**
** 03-Jan-2000 (uk$so01)
**    Created
** 29-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Management the quote string and wording.
** 30-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Management the quote string (in auditdb command line).
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/


#if !defined(RCRDTOOL_HEADER)
#define RCRDTOOL_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "logadata.h"
#include "dmlbase.h"
#include "dglishdr.h"

void RCRD_DisplayTransaction(CuListCtrl& cListCtrl, CTypedPtrList < CObList, CaBaseTransactionItemData* >* pList);
void RCRD_SelectItems(CuListCtrl& cListCtrl, UINT* pArrayItem, int nArraySize);
void RCRD_InitialOrder(CuListCtrl& cListCtrl, CTypedPtrList < CObList, CaBaseTransactionItemData* >* pList);
void IJA_ColorItem (CuListCtrl* pCtrl, int idx, COLORREF rgb);
//
// iItem = -1: Update all items if pTr = NULL
//           : Insert item at the last if pTr is not NULL.
// iItem = n : Update the nth item if if pTr = NULL.
//           : Insert item at nth if pTr is not NULL.
void RCRD_UdateDisplayList(CuListCtrl& cListCtrl, int iItem, CaBaseTransactionItemData* pTr);

//
//  Generate the Declare temporary table syntax:
//  IF bDeclareGlobal = TRUE (default), then use the declare global table
//  Else use the create table syntax.
//  --------------------------------------------
BOOL RCRD_GenerateTemporaryTableSyntax(
    LPCTSTR lpszTableName, 
    CTypedPtrList<CObList, CaColumn*>* pListColumn, 
    CString& strSyntax, BOOL bDeclareGlobal=TRUE);
//
//  Generate the AUDITDB Command with option:
//   -table = tablename {,tablename}
//   -file  = filename {,filename} ...
BOOL RCRD_GenerateAuditdbSyntax(
    CaQueryTransactionInfo* pQueryInfo,
    CTypedPtrList < CObList, CaIjaTable* >& listTable,
    CStringList& listFile,
    CString& strSyntax, LPCTSTR lpszConnectedUser, BOOL bGenerateSyntaxAdbToFst=FALSE);

//
// Check validation: for the selected transactions before/after update,
// the couple before update and after update must be selected !
// NOTE: for convenience, this function display a message box !!!
BOOL RCRD_CheckValidateTransaction (CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr);

//
// lpszDlgName = ("REDO", "RECOVER")
// ltr : initial order (list) of transactions
// This function return TRUE, if checked is OK, and FALSE if checked is not OK
// This function displays the WARNING message !
BOOL RCRD_CheckOrder(
    LPCTSTR lpszDlgName, 
    CuListCtrl& cListCtrl, 
    CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr);

//
// lParam1: the CaRule*
// lParam2: item index of list control
void CALLBACK RCRD_DisplayRule (CListCtrl* pList, LPARAM lParam1, LPARAM lParam2);
//
// lParam1: the CaConstraint*
// lParam2: item index of list control
void CALLBACK RCRD_DisplayConstraint (CListCtrl* pList, LPARAM lParam1, LPARAM lParam2);
//
// pParent:owner of the property frame.
// pPropFrame: [in,out] property frame object
// nTabSel: the current selection of Tab control of Rues and Constraints (0: Rule, 1: Constraint)
// wParam: 0 -> not force to create property frame. 
//         1 -> force to create property frame.
// lParam: pointer to the CaDBObject. (CaRule or CaConstraint)
long RCRD_OnDisplayProperty (CWnd* pParent, CfMiniFrame*& pPropFrame, int nTabSel, WPARAM wParam, LPARAM lParam);


void RCRD_SelchangeTabRuleConstraint(
    CuDlgListHeader*  pPageRule,
    CuDlgListHeader*  pPageIntegrity,
    CuDlgListHeader*  pPageConstraint,
    CuDlgListHeader*& pCurrentPage,
    CTabCtrl* pTabCtrl);

void RCRD_TabRuleConstraint (
    CuDlgListHeader*& pPageRule,
    CuDlgListHeader*& pPageIntegrity,
    CuDlgListHeader*& pPageConstraint,
    CuDlgListHeader*& pCurrentPage,
    CTabCtrl* pTab,
    CaQueryTransactionInfo* pQueryTransactionInfo,
    CTypedPtrList < CObList, CaDBObject* >& listRule,
    CTypedPtrList < CObList, CaDBObject* >& listIntegrity,
    CTypedPtrList < CObList, CaDBObject* >& listConstraint,
    CTypedPtrList < CObList, CaBaseTransactionItemData* >& listTransaction,
    CWnd* pCaller);

class CaTemporarySession;
class CaBaseRecoverRedoParam: public CObject
{
public:
	CaBaseRecoverRedoParam();
	~CaBaseRecoverRedoParam(){}
	BOOL PrepareData(CaRecoverRedoInformation& recover);
	void SetSession(CaTemporarySession* pSession){m_pSession = pSession;}

	CaQueryTransactionInfo* m_pQueryTransactionInfo;
	CTypedPtrList < CObList, CaBaseTransactionItemData* > m_listTransaction;
	CString m_strConnectedUser;
	CString m_strScript;
	BOOL m_bWholeTransaction;
	BOOL m_bScanJournal;
	BOOL m_bNoRule;
	BOOL m_bImpersonateUser;
	BOOL m_bErrorOnAffectedRowCount;

	//
	// Session from the outside:
	CaTemporarySession* m_pSession;
};


class CaRecoverParam: public CaBaseRecoverRedoParam
{
public:
	CaRecoverParam();
	~CaRecoverParam(){}

	BOOL Recover(CString& strError);
	BOOL GenerateScript (CString& strError, BOOL &bRoundingErrorsPossibleInText);
	BOOL RecoverNowScript (CString& strError);

};

class CaRedoParam: public CaBaseRecoverRedoParam
{
public:
	CaRedoParam();
	~CaRedoParam(){}

	BOOL Redo(CString& strError);
	BOOL GenerateScript (CString& strError, BOOL &bRoundingErrorsPossibleInText);
	BOOL RedoNowScript (CString& strError);

	CString m_strNode;
	CString m_strDatabase;

};

//
// Check to see if Table 'lpszTable' is caintained in list of constarints:
BOOL RCRD_IsTableHasConstraint(
    LPCTSTR lpszTable,
    LPCTSTR lpszTableOwner,
    CTypedPtrList < CObList, CaDBObject* >& listConstraint);

//
// Replace the non-formal character in file name by the under score '_'
void MakeFileName(CString& strItem);

#endif // RCRDTOOL_HEADER