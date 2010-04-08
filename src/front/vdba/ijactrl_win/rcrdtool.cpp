/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : rcrdtool.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Common tools for dialog "Recover" and "Redo"
**
** History:
**
** 03-Jan-2000 (uk$so01)
**    Created
** 23-Apr-2001 (uk$so01)
**    Bug #104487, 
**    When creating the temporary table, make sure to use the exact type based on its length.
**    If the type is integer and its length is 2 then use smallint or int2.
**    If the type is float and its length is 4 then use float(4).
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 29-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Management the quote string and wording.
** 30-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Management the quote string (in auditdb command line).
** 14-Jun-2001 (noifr01)
**    (bug 104954) generate the grantees list for the the temporary tables
** 08-Mar-2002 (noifr01)
**    (bug 107292)
**    embed filenames with "\" and \"" in -file= command level syntax
** 07-May-2002 (hanje04)
**    Embedding -file option on auditdb syntax which "\" and \"" causes
**    auditdb to fail on UNIX. Just wrap in \" for MAINWIN.
** 01-Aug-2002 (uk$so01)
**    BUG #108434, When the DBMS shows float with length 8 in iicolumns table,
**    ija should generate float or float8 instead of float(8).
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 02-Feb-2004 (noifr01)
**    removed unused header invocation that was causing problem with new 
**    versions of the compiler (with iostream libraries)
** 03-Aug-2004 (uk$so01)
**    BUG #112768, ISSUE 13480140, The query has been issued outside of a DBMS session.
**/

#include "stdafx.h"
#include "ijactrl.h"
#include "rcrdtool.h"
#include "ijalldml.h"
#include "ijactdml.h"
#include "ijadmlll.h"
#include "ijactdml.h"
#include "dmlbase.h"
#include "dmlrule.h"
#include "dmlconst.h"
#include "dmlinteg.h"
#include "tlsfunct.h"
#include "ingobdml.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static LPCTSTR cstrIngresSpecialChars = _T("$&*:,\"=/<>()-%.+?;' |");
void MakeFileName(CString& strItem)
{
	int nFind = strItem.FindOneOf (cstrIngresSpecialChars);
	while (nFind != -1)
	{
		strItem.SetAt (nFind, _T('_'));
		nFind = strItem.FindOneOf (cstrIngresSpecialChars);
	}
}


void IJA_ColorItem (CuListCtrl* pCtrl, int idx, COLORREF rgb)
{
	if (theApp.m_property.m_bPaintForegroundTransaction)
		pCtrl->SetFgColor (idx, rgb);
	else
		pCtrl->SetBgColor (idx, rgb);
}

void RCRD_SelectItems(CuListCtrl& cListCtrl, UINT* pArrayItem, int nArraySize)
{
	CWaitCursor doWaitCursor;
	try
	{
		if (!pArrayItem)
			return;
		if (nArraySize == 0)
			return;
		int i, nFound = -1;
		LV_FINDINFO lvfindinfo;
		memset (&lvfindinfo, 0, sizeof (LV_FINDINFO));
		lvfindinfo.flags = LVFI_PARAM;

		for (i=0; i<nArraySize; i++)
		{
			// Find the Transaction ID in the Main list of Transaction:
			lvfindinfo.lParam   = (LPARAM)pArrayItem[i];
			if (lvfindinfo.lParam == 0)
				continue;
			nFound = cListCtrl.FindItem (&lvfindinfo);
			ASSERT (nFound != -1);
			if (nFound != -1)
				cListCtrl.SetItemState (nFound, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	catch (...)
	{
		TRACE0 ("Raise Exeption: RCRD_SelectItems\n");
	}
}


void RCRD_DisplayTransaction(CuListCtrl& cListCtrl, CTypedPtrList < CObList, CaBaseTransactionItemData* >* pList)
{
	int nCount = 0, idx = -1;
	CString strTable;
	CTypedPtrList < CObList, CaBaseTransactionItemData* >& list = *pList;
	POSITION pos = list.GetHeadPosition();
	while (pos != NULL)
	{
		CaBaseTransactionItemData* pTr = list.GetNext(pos);
		if (pTr)
		{
			RCRD_UdateDisplayList(cListCtrl, -1, pTr);
		}
	}
}

void RCRD_InitialOrder(CuListCtrl& cListCtrl, CTypedPtrList < CObList, CaBaseTransactionItemData* >* pList)
{
	CWaitCursor doWaitCursor;
	try
	{
		UINT* pArraySelectedItem = NULL;
		int idx = 0;
		int i, nCount, nSelCount = cListCtrl.GetSelectedCount();
		if (nSelCount > 0)
		{
			pArraySelectedItem = new UINT [nSelCount];
			memset (pArraySelectedItem, 0, (UINT)nSelCount);
		}
		nCount = cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
			{
				pArraySelectedItem[idx] = (UINT)cListCtrl.GetItemData(i);
				idx++;
			}
		}

		cListCtrl.DeleteAllItems();
		RCRD_DisplayTransaction(cListCtrl, pList);
		RCRD_SelectItems (cListCtrl, pArraySelectedItem, nSelCount);
		if (pArraySelectedItem)
			delete [] pArraySelectedItem;
	}
	catch (...)
	{
		TRACE0 ("Raise Exeption: RCRD_InitialOrder\n");
	}
}

//
// iItem = -1: Update all items if pTr = NULL
//           : Insert item at the last if pTr is not NULL.
// iItem = n : Update the nth item if  pTr = NULL.
//           : Insert item at nth if pTr is not NULL.
void RCRD_UdateDisplayList(CuListCtrl& cListCtrl, int iItem, CaBaseTransactionItemData* pTr)
{
	int idx  =-1;
	int i, nCount = cListCtrl.GetItemCount();
	CString strTable;
	CaBaseTransactionItemData* pItem = NULL;
	
	if (pTr)
	{
		//
		// Insert new Item:
		int nPos = (iItem == -1)? nCount: iItem;
		int nImage = pTr->GetImageId();

		idx = cListCtrl.InsertItem (nPos, _T(""), nImage);
		if (idx != -1)
		{
			cListCtrl.SetItemData (idx, (DWORD)pTr);
			AfxFormatString2 (strTable, IDS_OWNERxITEM, (LPCTSTR)pTr->GetTableOwner(), (LPCTSTR)pTr->GetTable());

			switch (pTr->GetAction())
			{
			case T_DELETE:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_DELETE());
				break;
			case T_INSERT:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_INSERT());
				break;
			case T_BEFOREUPDATE:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_BEFOREUPDATE());
				break;
			case T_AFTERUPDATE:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_AFTEREUPDATE());
				break;
			default:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_OTHERS());
				break;
			}

			int nHeader = 0;
			CHeaderCtrl* pHeaderCtrl = cListCtrl.GetHeaderCtrl( );
			if (pHeaderCtrl)
				nHeader = pHeaderCtrl->GetItemCount();
			ASSERT (nHeader == 6); 

			cListCtrl.SetItemText (idx, 0, pTr->GetStrTransactionID());
			cListCtrl.SetItemText (idx, 1, pTr->GetLsn());
			cListCtrl.SetItemText (idx, 2, pTr->GetUser());
			cListCtrl.SetItemText (idx, 3, pTr->GetStrAction());
			cListCtrl.SetItemText (idx, 4, strTable);
			cListCtrl.SetItemText (idx, 5, pTr->GetData());
		}
	}
	else
	{
		//
		// Update the existing Items:
		int nStart = (iItem == -1)? 0: iItem;
		int nStop  = (iItem == -1)? nCount: iItem;
		for (i=nStart; i<nStop; i++)
		{
			pTr = (CaBaseTransactionItemData*)cListCtrl.GetItemData(i);
			ASSERT (pTr);
			if (!pTr)
				continue;
			AfxFormatString2 (strTable, IDS_OWNERxITEM, (LPCTSTR)pTr->GetTableOwner(), (LPCTSTR)pTr->GetTable());

			switch (pTr->GetAction())
			{
			case T_DELETE:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_DELETE());
				break;
			case T_INSERT:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_INSERT());
				break;
			case T_BEFOREUPDATE:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_BEFOREUPDATE());
				break;
			case T_AFTERUPDATE:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_AFTEREUPDATE());
				break;
			default:
				IJA_ColorItem(&cListCtrl, idx, theApp.m_property.TRGB_OTHERS());
				break;
			}
			
			int nHeader = 0;
			CHeaderCtrl* pHeaderCtrl = cListCtrl.GetHeaderCtrl( );
			if (pHeaderCtrl)
				nHeader = pHeaderCtrl->GetItemCount();
			ASSERT (nHeader == 6); 

			cListCtrl.SetItemText (idx, 0, pTr->GetStrTransactionID());
			cListCtrl.SetItemText (idx, 1, pTr->GetLsn());
			cListCtrl.SetItemText (idx, 2, pTr->GetUser());
			cListCtrl.SetItemText (idx, 3, pTr->GetStrAction());
			cListCtrl.SetItemText (idx, 4, strTable);
			cListCtrl.SetItemText (idx, 5, pTr->GetData());
		}
	}
}

static BOOL FindTransaction (CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr, unsigned long lsnH, unsigned long lsnL, TxType tranType)
{
	unsigned long lCurlsnL, lCurlsnH;
	POSITION pos = ltr.GetHeadPosition();
	while (pos != NULL)
	{
		CaBaseTransactionItemData* pItem = ltr.GetNext (pos);
		pItem->GetLsn (lCurlsnH, lCurlsnL);
		if (lCurlsnL == lsnL && lCurlsnH == lsnH)
		{
			if (pItem->GetAction() == tranType)
				return TRUE;
		}
	}
	return FALSE;
}

//
// Check validation: for the selected transactions before/after update,
// the couple before update and after update must be selected !
// NOTE: for convenience, this function display a message box !!!
BOOL RCRD_CheckValidateTransaction (CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr)
{
	BOOL bNoError = TRUE;
	POSITION pos = ltr.GetHeadPosition();
	while (pos != NULL)
	{
		CaBaseTransactionItemData* pItem = ltr.GetNext (pos);
		if (pItem->GetAction() == T_BEFOREUPDATE)
		{
			unsigned long lsnL, lsnH;
			pItem->GetLsn (lsnH, lsnL);
			if (!FindTransaction (ltr, lsnH, lsnL, T_AFTERUPDATE))
			{
				bNoError = FALSE;
				break;
			}
		}
		else
		if (pItem->GetAction() == T_AFTERUPDATE)
		{
			unsigned long lsnL, lsnH;
			pItem->GetLsn (lsnH, lsnL);
			if (!FindTransaction (ltr, lsnH, lsnL, T_BEFOREUPDATE))
			{
				bNoError = FALSE;
				break;
			}
		}
	}

	if (!bNoError)
	{
		//
		// You cannot select a 'before update' or 'after update' row without selecting also the 'after update'\n
		// or 'before update' row corresponding to the same LSN / row change.
		CString strMsg;
		strMsg.LoadString(IDS_SELECT_B4xAFTER_UPDATE);

		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
	return bNoError;
}


//
// lpszDlgName = ("REDO", "RECOVER")
// ltr : initial order (list) of transactions
// This function return TRUE, if checked is OK, and FALSE if checked is not OK
// This function displays the WARNING message !
BOOL RCRD_CheckOrder(
    LPCTSTR lpszDlgName, 
    CuListCtrl& cListCtrl, 
    CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr)
{
	int nAnswer = -1;
	//
	// The order of individual row changes, within some transactions, has been changed.\nContinue ?
	CString strMsg;
	strMsg.LoadString(IDS_RECOVERxREDO_ORDER_TRANS_CHANGED);

	if (_tcscmp (lpszDlgName, _T("REDO")) == 0)
	{
	}
	else
	if (_tcscmp (lpszDlgName, _T("RECOVER")) == 0)
	{
	}
	else
	{
		ASSERT (FALSE);
	}

	ASSERT (cListCtrl.GetItemCount() == ltr.GetCount());
	if (cListCtrl.GetItemCount() != ltr.GetCount())
	{
		nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
		if (nAnswer == IDNO)
			return FALSE;
	}

	int idx = 0, nCount = cListCtrl.GetItemCount();
	POSITION pos = ltr.GetHeadPosition();
	while (pos != NULL && idx < nCount && nAnswer != IDYES)
	{
		CaBaseTransactionItemData* pItem = ltr.GetNext (pos);
		if (pItem != (CaBaseTransactionItemData*)cListCtrl.GetItemData (idx))
		{
			nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
			if (nAnswer == IDNO)
				return FALSE;
		}
		idx++;
	}

	return TRUE;
}



//
// Recover Redo Base:
CaBaseRecoverRedoParam::CaBaseRecoverRedoParam()
{
	m_bWholeTransaction = TRUE;
	m_bScanJournal = FALSE;
	m_bNoRule = FALSE;
	m_bImpersonateUser = FALSE;
	m_bErrorOnAffectedRowCount = TRUE;

	m_pQueryTransactionInfo = NULL;
	m_strConnectedUser = _T("");
	m_strScript = _T("");
	m_pSession = NULL;
}

BOOL CaBaseRecoverRedoParam::PrepareData(CaRecoverRedoInformation& recover)
{
	BOOL bError = FALSE;

	ASSERT (m_pQueryTransactionInfo);
	if (!m_pQueryTransactionInfo)
		return FALSE;
	recover.m_pListTransaction = &m_listTransaction;
	recover.m_pQueryTransactionInfo = m_pQueryTransactionInfo;
	recover.m_bScanJournal = m_bScanJournal;
	recover.m_bNoRule = m_bNoRule;
	recover.m_bImpersonateUser = m_bImpersonateUser;
	recover.m_bErrorOnAffectedRowCount = m_bErrorOnAffectedRowCount;

	//
	// Prepare the list of tables 'listTable' involved in recovery:
	CString strTable;
	CString strTableOwner;
	CTypedPtrList < CObList, CaIjaTable* >& listTable = recover.m_listTable;

	POSITION pos = m_listTransaction.GetHeadPosition();
	while (pos != NULL)
	{
		CaBaseTransactionItemData* pBase = m_listTransaction.GetNext (pos);
		if (!isUndoRedoable(pBase->GetAction()))   /* don't include tables for non insert/update/delete statements*/
					continue;

		strTable = pBase->GetTable();
		strTableOwner = pBase->GetTableOwner();
		POSITION px = listTable.GetHeadPosition();
		BOOL bExist = FALSE;
		while (px != NULL)
		{
			CaIjaTable* pObj = listTable.GetNext (px);
			if (strTable.CompareNoCase (pObj->GetItem()) == 0 && strTableOwner.CompareNoCase (pObj->GetOwner()) == 0)
			{
				//
				// Table name have been marked !:
				bExist = TRUE;
				break;
			}
		}

		if (!bExist)
		{
			listTable.AddTail (new CaIjaTable(strTable, strTableOwner));
		}
	}

	CString strGenTableName;
	CaQueryTransactionInfo qinfo = *m_pQueryTransactionInfo;
	CTypedPtrList < CObList, CaDBObject* >& listTemporaryTable = recover.m_listTempTable;

	pos = listTable.GetHeadPosition();
	while (pos != NULL)
	{
		CaIjaTable* pTable = listTable.GetNext (pos);
		qinfo.SetTable (pTable->GetItem(), pTable->GetOwner());
		CTypedPtrList<CObList, CaColumn*>& listColumn = pTable->m_listColumn;
		if (!IJA_QueryColumn (&qinfo, listColumn, FALSE))
		{
			//
			// Failed to query the list of table's columns.
			CString strMsg;
			strMsg.LoadString(IDS_FAIL_TO_QUERY_TABLE_COLUMNS);

			AfxMessageBox (strMsg);
			return FALSE;
		}
		
		CString csGranteeList;
		POSITION pos1 = m_listTransaction.GetHeadPosition();
		while (pos1 != NULL)
		{
			CaBaseTransactionItemData* pBase = m_listTransaction.GetNext (pos1);
			if (!isUndoRedoable(pBase->GetAction()))   /* don't include tables for non insert/update/delete statements*/
						continue;
			strTable = pBase->GetTable();
			strTableOwner = pBase->GetTableOwner();
			if (strTable.CompareNoCase (pTable->GetItem()) == 0 && strTableOwner.CompareNoCase (pTable->GetOwner()) == 0) {
				/* don't grant a given user more than once */
				POSITION pos2 = m_listTransaction.GetHeadPosition();
				BOOL bAlreadyGranted = FALSE;
				while (pos2 != NULL)
				{
					CaBaseTransactionItemData* pBase2 = m_listTransaction.GetNext (pos2);
					if (pBase2 == pBase)
						break; /* re-scan to be stopped at previous item than current one */
					if (!isUndoRedoable(pBase2->GetAction()))  /* don't include tables for non insert/update/delete statements*/
						continue;
					if (pBase2->GetTable().CompareNoCase (pBase->GetTable()) == 0 && 
						pBase2->GetTableOwner().CompareNoCase (pBase->GetTableOwner()) == 0 &&
						pBase2->GetUser().CompareNoCase(pBase->GetUser())== 0) {
						bAlreadyGranted = TRUE;
						break;
					}
				}
				if (!bAlreadyGranted) {
					if (!csGranteeList.IsEmpty())
						csGranteeList+=',';
					csGranteeList+=pBase->GetUser();
				}
			}
		}
		
		BOOL bOK = IJA_TableAuditdbOutput (&qinfo, m_pSession, strGenTableName, &listColumn, csGranteeList);
		if (!bOK)
			return FALSE;
		if (m_pSession->IsLocalNode())
			listTemporaryTable.AddTail (new CaDBObject (strGenTableName, _T("session")));
		else
		{
			//
			// The current connect user will be the creator ot temporary table:
			listTemporaryTable.AddTail (new CaDBObject (strGenTableName, m_strConnectedUser));
		}
	}

	return TRUE;
}



//
// Recover:
CaRecoverParam::CaRecoverParam():CaBaseRecoverRedoParam()
{
}


BOOL CaRecoverParam::Recover(CString& strError)
{
	CaRecoverRedoInformation recover (m_pSession);
	if (!PrepareData (recover))
		return FALSE;
	BOOL bDummy; // applies only to Generate Script
	BOOL bOk = IJA_RecoverRedoAction (ACTION_RECOVER, &recover, m_strScript,strError,bDummy);
	return bOk;
}

BOOL CaRecoverParam::GenerateScript (CString& strError, BOOL &bRoundingErrorsPossibleInText)
{
	CaRecoverRedoInformation recover(m_pSession);
	if (!PrepareData (recover))
		return FALSE;
	BOOL bOk = IJA_RecoverRedoAction (ACTION_RECOVER_SCRIPT, &recover, m_strScript,strError,bRoundingErrorsPossibleInText);

	return bOk;
}

BOOL CaRecoverParam::RecoverNowScript (CString& strError)
{
	CaRecoverRedoInformation recover(m_pSession);
	if (!PrepareData (recover))
		return FALSE;
	BOOL bDummy; // applies only to Generate Script
	BOOL bOk = IJA_RecoverRedoAction (ACTION_RECOVERNOW_SCRIPT, &recover, m_strScript, strError,bDummy);
	return bOk;
}



//
// Redo:
CaRedoParam::CaRedoParam():CaBaseRecoverRedoParam()
{
	m_strNode = _T("");
	m_strDatabase = _T("");
}

BOOL CaRedoParam::Redo(CString& strError)
{
	CaRecoverRedoInformation redo(m_pSession);
	if (!PrepareData (redo))
		return FALSE;
	BOOL bDummy; // applies only to Generate Script
	BOOL bOk = IJA_RecoverRedoAction (ACTION_REDO, &redo, m_strScript, strError, bDummy);
	return bOk;
}


BOOL CaRedoParam::GenerateScript (CString& strError, BOOL &bRoundingErrorsPossibleInText)
{
	CaRecoverRedoInformation redo(m_pSession);
	if (!PrepareData (redo))
		return FALSE;
	BOOL bOk = IJA_RecoverRedoAction (ACTION_REDO_SCRIPT, &redo, m_strScript, strError,bRoundingErrorsPossibleInText);
	return bOk;
}

BOOL CaRedoParam::RedoNowScript (CString& strError)
{
	CaRecoverRedoInformation redo(m_pSession);
	if (!PrepareData (redo))
		return FALSE;
	BOOL bDummy; // applies only to Generate Script
	BOOL bOk = IJA_RecoverRedoAction (ACTION_REDONOW_SCRIPT, &redo, m_strScript, strError,bDummy);
	return bOk;
}


//
//  Generate the Declare temporary table syntax:
//  IF bDeclareGlobal = TRUE (default), then use the declare global table
//  Else use the create table syntax.
//  --------------------------------------------
BOOL RCRD_GenerateTemporaryTableSyntax( LPCTSTR lpszTableName, CTypedPtrList<CObList, CaColumn*>* pListColumn, CString& strSyntax, BOOL bDeclareGlobal)
{
	//
	// This table is created for the target of COPY TABLE () FROM 'myfile.trl'.
	// where myfile.trl is generated by the AUDITDB command.
	// The table must have the following columns and in that ORDER:
	// 
	// 1  LSNHIGH       integer not null with default,  (4-bytes integer low order of LSN)
	// 2  LSNLOW     integer not null with default,  (4-bytes integer high order of LSN)
	// 3  TIDNO        integer not null with default,
	// 4  DATE         date not null with default,
	// 5  USERNAME     char(32) not null with default,
	// 6  OPERATION    char(8) not null with default,
	// 7  TRANID1      integer not null with default,  (4-bytes integer high order of TRANSACTION ID)
	// 8  TRANID2      integer not null with default,  (4-bytes integer low  order of TRANSACTION ID)
	// 9  TABLE_ID1    integer not null with default,  (4-bytes integer)
	// 10 TABLE_ID2    integer not null with default,  (4-bytes integer)
	// XX The columns of the audited table.
	CString strAuditCols = _T("");
	CString str1 = _T("CREATE TABLE %s (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s");
	if (bDeclareGlobal)
		str1 = _T("DECLARE GLOBAL TEMPORARY TABLE SESSION.%s (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s");

	strSyntax.Format (
		(LPCTSTR)str1,
		(LPCTSTR)lpszTableName,
		_T("LSNHIGH      integer not null with default"),
		_T("LSNLOW       integer not null with default"),
		_T("TIDNO        integer not null with default"),
		_T("DATE         date not null with default"),
		_T("USERNAME     char(32) not null with default"),
		_T("OPERATION    char(8) not null with default"),
		_T("TRANID1      integer not null with default"),
		_T("TRANID2      integer not null with default"),
		_T("TABLE_ID1    integer not null with default"),
		_T("TABLE_ID2    integer not null with default")
		);
	ASSERT (pListColumn);
	if (!pListColumn)
		return FALSE;
	//
	// Generate the audited table columns, the generated columns are prefixed by 'ija_'
	// so that their names will not be the same as the  first 10 columns:
	int nNeedLen = 0;
	CString strColType = _T("");
	CString strCol = _T("");
	CString strLength = _T("");
	CaColumn* pCol = NULL;
	POSITION pos = pListColumn->GetHeadPosition();
	while (pos != NULL)
	{
		pCol = pListColumn->GetNext (pos);
		strLength = _T("");
		strColType = pCol->GetTypeStr();
		strColType.MakeLower();

		nNeedLen = INGRESII_llNeedLength(pCol->GetType());
		if (nNeedLen == 1 || nNeedLen == 2)
		{
			strLength.Format (_T("(%d)"), pCol->GetLength());
			if (nNeedLen == 2 && strColType.Find(_T("float")) == 0)
			{
				switch(pCol->GetLength())
				{
				case 4:
					strColType = _T("float");  // DECLARE GLOBAL TEMPORARY does not accept float4 but float(4)
					break;
				case 8:
					strLength  = _T("");
					strColType = _T("float8"); // float & float8 have the same output in iicolumns, we choose float8.
					break;
				default:
					break;
				}
			}
		}
		else
		if (nNeedLen == 3)
			strLength.Format (_T("(%d, %d)"), pCol->GetLength(), pCol->GetScale());

		if (pCol->IsNullable())
		{
			if (pCol->IsDefault())
			{
				strCol.Format (
					_T(", ija_%s %s%s with null with default"), 
					(LPCTSTR)pCol->GetItem(), 
					(LPCTSTR)strColType,
					(LPCTSTR)strLength);
			}
			else
			{
				strCol.Format (
					_T(", ija_%s %s%s with null not default"), 
					(LPCTSTR)pCol->GetItem(), 
					(LPCTSTR)strColType,
					(LPCTSTR)strLength);
			}
		}
		else
		{
			if (pCol->IsDefault())
			{
				strCol.Format (
					_T(", ija_%s %s%s not null with default"), 
					(LPCTSTR)pCol->GetItem(), 
					(LPCTSTR)strColType,
					(LPCTSTR)strLength);
			}
			else
			{
				strCol.Format (
					_T(", ija_%s %s%s not null not default"), 
					(LPCTSTR)pCol->GetItem(), 
					(LPCTSTR)strColType,
					(LPCTSTR)strLength);
			}
		}
		
		strAuditCols+= strCol;

	}
	strAuditCols += _T(")");
	strSyntax+= strAuditCols;
	str1=_T(" with nojournaling");
	if (bDeclareGlobal)
		str1=_T(" ON COMMIT PRESERVE ROWS WITH NORECOVERY");
	strSyntax+= str1;
	return TRUE;
}


//
//  Generate the AUDITDB Command with option:
//  ...
//   -table = tablename {,tablename}
//   -file  = filename {,filename} ...
BOOL RCRD_GenerateAuditdbSyntax(
    CaQueryTransactionInfo* pQueryInfo,
    CTypedPtrList < CObList, CaIjaTable* >& listTable,
    CStringList& listFile,
    CString& strSyntax, LPCTSTR lpszConnectedUser, BOOL bGenerateSyntaxAdbToFst)
{
	CString strDatabase;
	CString strDatabaseOwner;
	CString strTable;
	CString strFile;
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
	if (pQueryInfo->GetAfterCheckPoint() && pQueryInfo->GetCheckPointNumber() > 0)
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

	//
	// Generate the list of files and tables:
	ASSERT (listTable.GetCount() == listFile.GetCount() && listTable.GetCount() <= 64);
	if (listTable.GetCount() != listFile.GetCount() || listTable.GetCount() > 64 || listFile.GetCount() > 64)
	{
		//
		// In auditdb,\nthe number of specified tables must be equal to the number of specifed files 
		// and at most equal to 64.
		CString strMsg;
		strMsg.LoadString (IDS_AUDITDB_SYNTAX1);
		
		AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	strTable = _T("");
	BOOL bOne = TRUE;
	POSITION pos = listTable.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem;
		CaDBObject* pObj = listTable.GetNext (pos);
		if (!bOne)
			strTable += _T(',');

		if (theApp.m_bTableWithOwnerAllowed)
		{
			strItem = INGRESII_llQuoteIfNeeded(pObj->GetOwner(), _T('\"'), TRUE);
			strTable += strItem;
			strTable += _T('.');
		}

		strItem = INGRESII_llQuoteIfNeeded(pObj->GetItem(), _T('\"'), TRUE);
		strTable += strItem;
		bOne = FALSE;
	}

	strFile = _T("");
	bOne = TRUE;
	pos = listFile.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = listFile.GetNext (pos);
		if (!bOne)
			strFile += _T(',');
		strFile += strItem;
		bOne = FALSE;
	}

	if (bGenerateSyntaxAdbToFst)
	{
		strSyntax.Format (
			_T("%s %s -internal_data -aborted_transactions %s"),
			(LPCTSTR)strTable,
			(LPCTSTR)strDatabase,
			(LPCTSTR)strFlags);
	}
	else
	{
		strSyntax.Format (
#ifdef MAINWIN
			_T("auditdb %s -u%s -internal_data -aborted_transactions -table=%s -file=\"%s\" %s"),
#else
			_T("auditdb %s -u%s -internal_data -aborted_transactions -table=%s -file=\"\\\"%s\\\"\" %s"),
#endif

			(LPCTSTR)strDatabase,
			(LPCTSTR)lpszConnectedUser,
			(LPCTSTR)strTable,
			(LPCTSTR)strFile,
			(LPCTSTR)strFlags);
	}
	TRACE1("SYNTAX=%s\n", strSyntax);
	return TRUE;
}


void CALLBACK RCRD_DisplayRule (CListCtrl* pList, LPARAM lParam1, LPARAM lParam2)
{
	CaIjaRule* pRule = (CaIjaRule*)lParam1;
	int nIdx = (int)lParam2;
	if (pRule == NULL || nIdx == -1)
		return;

	CString strTable;
	AfxFormatString2 (strTable, IDS_OWNERxITEM, (LPCTSTR)pRule->GetBaseTableOwner(), pRule->GetBaseTable());

	pList->SetItemText (nIdx, 0, pRule->GetName());
	pList->SetItemText (nIdx, 1, strTable);
}

void CALLBACK RCRD_DisplayConstraint (CListCtrl* pList, LPARAM lParam1, LPARAM lParam2)
{
	CaConstraint* pConstraint = (CaConstraint*)lParam1;
	int nIdx = (int)lParam2;
	if (pConstraint == NULL || nIdx == -1)
		return;

	CString strTable;
	AfxFormatString2 (strTable, IDS_OWNERxITEM, (LPCTSTR)pConstraint->GetOwner(), pConstraint->GetTable());
	pList->SetItemText (nIdx, 0, pConstraint->GetName());
	pList->SetItemText (nIdx, 1, strTable);
	pList->SetItemText (nIdx, 2, pConstraint->GetType());
}

void CALLBACK RCRD_DisplayIntegrity (CListCtrl* pList, LPARAM lParam1, LPARAM lParam2)
{
	CaIntegrityDetail* pObj = (CaIntegrityDetail*)lParam1;
	int nIdx = (int)lParam2;
	if (pObj == NULL || nIdx == -1)
		return;

	CString strTable;
	AfxFormatString2 (strTable, IDS_OWNERxITEM, (LPCTSTR)pObj->GetBaseTableOwner(), pObj->GetBaseTable());
	pList->SetItemText (nIdx, 0, pObj->GetName());
	pList->SetItemText (nIdx, 1, strTable);
}

static void RCRD_OnProperties(CWnd* pParent, CfMiniFrame*& pPropFrame)
{
	if (pPropFrame == NULL)
	{
		pPropFrame = new CfMiniFrame();
		CRect rect(0, 0, 0, 0);
		CString strTitle;
		VERIFY(strTitle.LoadString(IDS_PROPSHT_CAPTION));

		if (!pPropFrame->Create(NULL, strTitle,
			WS_THICKFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU, rect, pParent))
		{
			delete pPropFrame;
			pPropFrame = NULL;
			return;
		}
		pPropFrame->CenterWindow();
	}

	// Before unhiding the modeless property sheet, update its
	// settings appropriately.  For example, if you are reflecting
	// the state of the currently selected item, pick up that
	// information from the active view and change the property
	// sheet settings now.

	if (pPropFrame != NULL && !pPropFrame->IsWindowVisible())
		pPropFrame->ShowWindow(SW_SHOW);
}

long RCRD_OnDisplayProperty (CWnd* pParent, CfMiniFrame*& pPropFrame, int nTabSel, WPARAM wParam, LPARAM lParam)
{
	try
	{
#ifdef MAINWIN
		TCHAR tchszReturn [] = {0x0A, 0x00};
#else
		TCHAR tchszReturn [] = {0x0D, 0x0A, 0x00};
#endif
		int nMode = (int)wParam;
		CString strText = _T("");
		CString strTitle;
		CString strTabLabel;
		CaDBObject* pObj = (CaDBObject*)lParam;

		switch (nTabSel)
		{
		case 0: // Rule
			if (pObj->IsKindOf (RUNTIME_CLASS (CaRule)))
			{
				CaIjaRule* pRule = (CaIjaRule*)pObj;
				strText = pRule->GetDetailText();
				strText+= tchszReturn;
				strText+= tchszReturn;
				strText+= _T("PROCEDURE TEXT:");
				strText+= tchszReturn;
				CString strProcText = pRule->m_procedure.GetDetailText();
				RCTOOL_CR20x0D0x0A (strProcText);
				strText+= strProcText;
				strTitle = _T("Property for Rule ");
				strTitle+= pRule->GetName();
				strTabLabel = _T("Rule Text");
			}
			break;
		case 1: // Constraint
			if (pObj->IsKindOf (RUNTIME_CLASS (CaConstraint)))
			{
				CaConstraint* pConstr = (CaConstraint*)pObj;
				strText = pConstr->m_strText;
				strTitle = _T("Property for Constraint ");
				strTitle+= pConstr->GetName();
				strTabLabel = _T("Constraint Text");
			}
			break;
		case 2: // Integrity
			if (pObj->IsKindOf (RUNTIME_CLASS (CaIntegrity)))
			{
				CaIntegrity* pInteg = (CaIntegrity*)pObj;
				strText = pInteg->GetName();
				strTitle = _T("Property for Integrity ");
				strTabLabel = _T("Integrity Text");
			}
			break;
		default:
			ASSERT (FALSE);
			return 0;
			break;
		}

		switch (nMode)
		{
		case 0:
			if (pPropFrame && IsWindow (pPropFrame->m_hWnd) && pPropFrame->IsWindowVisible ())
			{
				pPropFrame->SetWindowText (strTitle);
				pPropFrame->SetText (strText, strTabLabel);
			}
			break;
		case 1:
			RCRD_OnProperties(pParent, pPropFrame);
			if (pPropFrame)
			{
				pPropFrame->SetWindowText (strTitle);
				pPropFrame->SetText (strText, strTabLabel);
			}
			break;
		default:
			break;
		}
	}
	catch (...)
	{
		TRACE0 ("Raise exception at: RCRD_OnDisplayProperty \n");
	}
	return 0;
}

void RCRD_SelchangeTabRuleConstraint(
    CuDlgListHeader*  pPageRule,
    CuDlgListHeader*  pPageIntegrity,
    CuDlgListHeader*  pPageConstraint,
    CuDlgListHeader*& pCurrentPage,
    CTabCtrl* pTabCtrl)
{
	ASSERT (pTabCtrl && pPageRule && pPageIntegrity && pPageConstraint);
	if (!(pTabCtrl && pPageRule && pPageIntegrity&& pPageConstraint))
		return;
	CRect r;
	int nSel = pTabCtrl->GetCurSel();

	if (pCurrentPage && IsWindow (pCurrentPage->m_hWnd))
		pCurrentPage->ShowWindow (SW_HIDE);
	switch (nSel)
	{
	case 0:
		pCurrentPage = pPageRule;
		break;
	case 1:
		pCurrentPage = pPageConstraint;
		break;
	case 2:
		pCurrentPage = pPageIntegrity;
		break;
	default:
		pCurrentPage = pPageRule;
		break;
	}
	pTabCtrl->GetClientRect (r);
	pTabCtrl->AdjustRect (FALSE, r);
	if (pCurrentPage && IsWindow (pCurrentPage->m_hWnd))
	{
		pCurrentPage->MoveWindow (r);
		pCurrentPage->ShowWindow(SW_SHOW);
	}
}

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
    CWnd* pCaller)
{
	//
	// Tab control containing list of rules & constraints:
	ASSERT (pTab);
	if (!pTab)
		return;
	const int NTAB = 3;
	UINT nTab [NTAB] = {IDS_HEADER_RULE, IDS_HEADER_CONSTRAINT, IDS_HEADER_INTEGRITY};
	CRect   r;
	CString strHeader;
	TC_ITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT;
	item.cchTextMax = 32;
	for (int i=0; i<NTAB; i++)
	{
		strHeader.LoadString (nTab[i]);
		item.pszText = (LPTSTR)(LPCTSTR)strHeader;
		pTab->InsertItem (i, &item);
	}

	LSCTRLHEADERPARAMS2 lspRule[2] =
	{
		{IDS_HEADER_RULENAME,          200,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_RULEONTABLE,       200,  LVCFMT_LEFT, FALSE}
	};
	LSCTRLHEADERPARAMS2 lspConstraint[3] =
	{
		{IDS_HEADER_CONSTRAINTNAME,    180,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_CONSTRAINTONTABLE, 180,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_CONSTRAINTTYPE,    120,  LVCFMT_LEFT, FALSE}
	};
	LSCTRLHEADERPARAMS2 lspIntegrity[2] =
	{
		{IDS_HEADER_INTEGRITYTEXT,     180,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_CONSTRAINTONTABLE, 180,  LVCFMT_LEFT, FALSE},
	};


	try
	{
		pPageRule = new CuDlgListHeader (pTab);
		pPageConstraint = new CuDlgListHeader (pTab);
		pPageIntegrity = new CuDlgListHeader (pTab);

		pPageRule->m_nHeaderCount   = 2;
		pPageRule->m_pArrayHeader = lspRule;
		pPageRule->m_lpfnDisplay  = RCRD_DisplayRule;
		pPageRule->m_pWndMessageHandler  = pCaller;

		pPageConstraint->m_nHeaderCount   = 3;
		pPageConstraint->m_pArrayHeader = lspConstraint;
		pPageConstraint->m_lpfnDisplay  = RCRD_DisplayConstraint;
		pPageConstraint->m_pWndMessageHandler  = pCaller;

		pPageIntegrity->m_nHeaderCount   = 2;
		pPageIntegrity->m_pArrayHeader = lspIntegrity;
		pPageIntegrity->m_lpfnDisplay  = RCRD_DisplayIntegrity;
		pPageIntegrity->m_pWndMessageHandler  = pCaller;

		pPageRule->Create (IDD_LISTHEADER, pTab);
		pPageConstraint->Create (IDD_LISTHEADER, pTab);
		pPageIntegrity->Create (IDD_LISTHEADER, pTab);

		pTab->SetCurSel (0);
		RCRD_SelchangeTabRuleConstraint(pPageRule, pPageIntegrity, pPageConstraint, pCurrentPage, pTab);

		//
		// At this point, we query the list of rules and constraints:
		CString strTable;
		CString strTableOwner;
		CString strItem;
		CString strDatabase;
		CString strDatabaseOwner;
		CStringList listDoneTable; // List of table of which we have already queried rule and constraints
		CaQueryTransactionInfo queryInfo = *pQueryTransactionInfo;
		CaBaseTransactionItemData* pTx = NULL;
		POSITION pos = listTransaction.GetHeadPosition();

		pQueryTransactionInfo->GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session(pQueryTransactionInfo->GetNode(), strDatabase);

		while (pos != NULL)
		{
			pTx = listTransaction.GetNext (pos);
			strTable = pTx->GetTable();
			strTableOwner = pTx->GetTableOwner();
			strItem.Format (_T("%s.%s"), (LPCTSTR)strTableOwner, (LPCTSTR)strTable);
			strItem.MakeLower();

			if (listDoneTable.Find (strItem) != NULL)
				continue; // Already queried !
			listDoneTable.AddTail (strItem);
			queryInfo.SetTable (strTable, strTableOwner);

			IJA_QueryRule (&queryInfo, listRule, &session);
			IJA_QueryConstraint (&queryInfo, listConstraint, &session);
			IJA_QueryIntegrity  (&queryInfo, listIntegrity, &session);
		}

		CaDBObject* pObj = NULL;
		pos = listRule.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = listRule.GetNext(pos);
			pPageRule->AddItem ((LPARAM)pObj);
		}

		pos = listConstraint.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = listConstraint.GetNext(pos);
			pPageConstraint->AddItem ((LPARAM)pObj);
		}

		pos = listIntegrity.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = listIntegrity.GetNext(pos);
			pPageIntegrity->AddItem ((LPARAM)pObj);
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch (...)
	{
		AfxMessageBox (_T("System error: RCRD_TabRuleConstraint, cannot display rules and constraints"));
	}
}

//
// Check to see if Table 'lpszTable' is caintained in list of constarints:
BOOL RCRD_IsTableHasConstraint(
    LPCTSTR lpszTable,
    LPCTSTR lpszTableOwner,
    CTypedPtrList < CObList, CaDBObject* >& listConstraint)
{
	BOOL bExist = FALSE;
	CaConstraint* pObj = NULL;

	POSITION pos = listConstraint.GetHeadPosition();
	while (pos != NULL)
	{
		CaConstraint* pObj = (CaConstraint*)listConstraint.GetNext (pos);
		if (pObj->GetTable().CompareNoCase(lpszTable) == 0 && pObj->GetTableOwner().CompareNoCase(lpszTableOwner) == 0)
		{
			bExist = TRUE;
			break;
		}
	}
	return bExist;
}
