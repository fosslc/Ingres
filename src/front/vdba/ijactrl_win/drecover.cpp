/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : drecover.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of Recover Dialog Box
**
** History:
**
** 28-Dec-1999 (uk$so01)
**    created
** 10-Oct-2000 (noifr01)
**    added the "scan journal to the end" feature
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 29-Jun-2001 (hanje04)
**    Added CString() around "" to stop build errors on Solaris
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 15-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/


#include "stdafx.h"
#include "ijactrl.h"
#include "drecover.h"
#include "rcrdtool.h"
#include "ijadmlll.h"
#include "ijactdml.h"
#include "parser.h"
#include "xscript.h"
#include "dlgmsbox.h"
#include "sqlselec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static int CALLBACK CompareSubItemInitialOrder (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC;
	CaListCtrlColorItemData* lpItem1 = (CaListCtrlColorItemData*)lParam1;
	CaListCtrlColorItemData* lpItem2 = (CaListCtrlColorItemData*)lParam2;
	if (lpItem1 == NULL || lpItem2 == NULL)
		return 0;
	CaBaseTransactionItemData* pItem1 = (CaBaseTransactionItemData*)lpItem1->m_lUserData;
	CaBaseTransactionItemData* pItem2 = (CaBaseTransactionItemData*)lpItem2->m_lUserData;

	SORTPARAMS*   pSr = (SORTPARAMS*)lParamSort;
	BOOL bAsc = pSr? pSr->m_bAsc: TRUE;

	if (pItem1 && pItem2)
	{
		CString strItem1 = pItem1->GetLsnEnd();
		CString strItem2 = pItem1->GetLsnEnd();
		strItem1 += pItem1->GetLsn();
		strItem2 += pItem2->GetLsn();
		nC = bAsc? 1 : -1;
		return nC * strItem1.CompareNoCase (strItem2);
	}
	else
	{
		nC = pItem1? 1: pItem2? -1: 0;
		return bAsc? nC: (-1)*nC;
	}

	return 0;
}

CxDlgRecover::CxDlgRecover(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRecover::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgRecover)
	m_bCheckScanJournal = TRUE;
	m_bNoRule = FALSE;
	m_bImpersonateUser = FALSE;
	m_bErrorOnAffectedRowCount = TRUE;
	//}}AFX_DATA_INIT
	m_pQueryTransactionInfo = NULL;
	m_pArrayItem = NULL;
	m_nArraySize= 0;

	m_pDlgListRules = NULL;
	m_pDlgListIntegrities = NULL;
	m_pDlgListConstraints = NULL;
	m_pCurrentPage = NULL;
	m_pPropFrame = NULL;

	m_strActionDelete.LoadString (IDS_AxDELETE); // DELETED
	m_strActionUpdate.LoadString (IDS_AxUPDATE); // UPDATED
	m_strActionAlter.LoadString  (IDS_AxALTER);  // ALTERED
	m_strActionDrop.LoadString   (IDS_AxDROP);   // DROPPED
	m_strActionMore.LoadString   (IDS_AxMORE);   // \nIn particular, Rules or Constraints on that table may have changed
}


void CxDlgRecover::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgRecover)
	DDX_Control(pDX, IDC_BUTTON6, m_cButtonInitialOrder);
	DDX_Control(pDX, IDC_BUTTON5, m_cButtonInitialSelection);
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonDown);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonUp);
	DDX_Check(pDX, IDC_CHECK1, m_bCheckScanJournal);
	DDX_Check(pDX, IDC_CHECK2, m_bNoRule);
	DDX_Check(pDX, IDC_CHECK3, m_bImpersonateUser);
	DDX_Check(pDX, IDC_CHECK7, m_bErrorOnAffectedRowCount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgRecover, CDialog)
	//{{AFX_MSG_MAP(CxDlgRecover)
	ON_BN_CLICKED(IDC_BUTTON5, OnButtonInitialSelection)
	ON_BN_CLICKED(IDC_BUTTON6, OnButtonInitialOrder)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonDown)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioWholeTransaction)
	ON_BN_CLICKED(IDOK, OnButtonRecoverNow)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonGenerateScript)
	ON_BN_CLICKED(IDC_BUTTON9, OnButtonRecoverNowScript)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LISTCTRLITEM_PROPERTY, OnDisplayProperty)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRecover message handlers

BOOL CxDlgRecover::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ImageList.Create (IDB_IJAIMAGELIST2, 16, 0, RGB(255,0,255));
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE, style);
	m_cListCtrl.SetFullRowSelected (TRUE);
	m_cListCtrl.SetLineSeperator(FALSE);
	m_cListCtrl.SetImageList  (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.ActivateColorItem();

	const int nHeader = 6;
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[nHeader] =
	{
		{IDS_HEADER_TRANSACTION,145, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_LSN,        145, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_USER,        70, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_ACTION,     100, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_TABLE,      120, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_DATA,       480, LVCFMT_LEFT, FALSE},
	};

	try
	{
		CString strHeader;
		memset (&lvcolumn, 0, sizeof (LV_COLUMN));
		lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
		for (int i=0; i<nHeader; i++)
		{
			strHeader.LoadString (lsp[i].m_nIDS);
			lvcolumn.fmt      = lsp[i].m_fmt;
			lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
			lvcolumn.iSubItem = i;
			lvcolumn.cx       = lsp[i].m_cxWidth;
			m_cListCtrl.InsertColumn (i, &lvcolumn); 
		}

		RCRD_DisplayTransaction(m_cListCtrl, &m_listTransaction);
		RCRD_SelectItems (m_cListCtrl, m_pArrayItem, m_nArraySize);
		EnableButtons();
#if !defined (SIMULATION)
		SORTPARAMS sp;
		sp.m_bAsc = TRUE;
		m_cListCtrl.SortItems(CompareSubItemInitialOrder, (LPARAM)&sp);
#endif
		//
		// Tab control containing list of rules & constraints:
		RCRD_TabRuleConstraint (
			m_pDlgListRules,
			m_pDlgListIntegrities,
			m_pDlgListConstraints,
			m_pCurrentPage,
			&m_cTab1,
			m_pQueryTransactionInfo,
			m_listRule,
			m_listIntegrity,
			m_listConstraint,
			m_listTransaction,
			this);

		//
		// ToolTip Management
		m_toolTip.Create(this);
		m_toolTip.Activate(TRUE);
		CString strTools;
		strTools.LoadString(IDS_TOOLSTIP_BUTTON_DOWN);
		m_toolTip.AddTool(&m_cButtonDown,            strTools);//_T("Move Item Down")
		strTools.LoadString(IDS_TOOLSTIP_BUTTON_UP);
		m_toolTip.AddTool(&m_cButtonUp,              strTools);// _T("Move Item Up")
		strTools.LoadString(IDS_TOOLSTIP_BUTTON_SEL);
		m_toolTip.AddTool(&m_cButtonInitialSelection,strTools);//_T("Default Selection")
		strTools.LoadString(IDS_TOOLSTIP_BUTTON_ORDER);
		m_toolTip.AddTool(&m_cButtonInitialOrder,    strTools);//_T("Default Order")
	}
	catch (...)
	{
		TRACE0 ("Raise exception at: CxDlgRecover::OnInitDialog()\n");
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CxDlgRecover::OnButtonInitialSelection() 
{
	CWaitCursor doWaitCursor;
	try
	{
		CTypedPtrList < CObList, CaBaseTransactionItemData* > listTransaction;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CaBaseTransactionItemData* pItem = (CaBaseTransactionItemData*)m_cListCtrl.GetItemData (i);
			listTransaction.AddTail(pItem);
		}
		m_cListCtrl.DeleteAllItems();
		RCRD_DisplayTransaction (m_cListCtrl, &listTransaction);
		RCRD_SelectItems (m_cListCtrl, m_pArrayItem, m_nArraySize);
		EnableButtons();
	}
	catch(...)
	{
		TRACE0 ("Raise exception: CxDlgRecover::OnButtonInitialSelection() \n");
	}
}

void CxDlgRecover::OnButtonInitialOrder() 
{
	RCRD_InitialOrder(m_cListCtrl, &m_listTransaction);
	EnableButtons();
}

void CxDlgRecover::OnButtonUp() 
{
	try
	{
		if (m_cListCtrl.GetSelectedCount() != 1)
			return;
		int i, nSelected = -1, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
			{
				nSelected = i;
				break;
			}
		}
		if (nSelected == -1 || nSelected == 0 || nCount == 1)
			return;
		//
		// Current Item
		CaBaseTransactionItemData* pItemCur= (CaBaseTransactionItemData*)m_cListCtrl.GetItemData (nSelected);
		//
		// Exchange Item Data:
		m_cListCtrl.DeleteItem (nSelected);
		RCRD_UdateDisplayList  (m_cListCtrl, nSelected-1, pItemCur);
		m_cListCtrl.SetItemState(nSelected-1, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		EnableButtons();
	}
	catch (...)
	{
		TRACE0 ("Raise exception at: CxDlgRecover::OnButtonUp()\n");
	}
}

void CxDlgRecover::OnButtonDown() 
{
	try
	{
		if (m_cListCtrl.GetSelectedCount() != 1)
			return;
		int i, nSelected = -1, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
			{
				nSelected = i;
				break;
			}
		}
		if (nSelected == -1 || nSelected >= (nCount -1))
			return;
		//
		// Current Item
		CaBaseTransactionItemData* pItemDown = (CaBaseTransactionItemData*)m_cListCtrl.GetItemData (nSelected+1);
		//
		// Exchange Item Data:
		m_cListCtrl.DeleteItem (nSelected+1);
		RCRD_UdateDisplayList  (m_cListCtrl, nSelected, pItemDown);
		EnableButtons();
	}
	catch (...)
	{
		TRACE0 ("Raise exception at: CxDlgRecover::OnButtonDown() \n");
	}
}

void CxDlgRecover::EnableButtons()
{
	BOOL bUpEnable   = FALSE;
	BOOL bDownEnable = FALSE;
	int i, nSelected = -1, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
		{
			nSelected = i;
			break;
		}
	}
	if (nSelected != -1 && m_cListCtrl.GetSelectedCount() == 1)
	{
		CaBaseTransactionItemData* pItemCur = (CaBaseTransactionItemData*)m_cListCtrl.GetItemData (nSelected);
		//
		// Can be moved down only if:
		// 1) nSelected < (count -1)
		// 2) TransID [nSelected] = TransID [nSelected+1]
		if (pItemCur && nSelected < (nCount-1))
		{
			CaBaseTransactionItemData* pItemDown = (CaBaseTransactionItemData*)m_cListCtrl.GetItemData (nSelected + 1);
			if (pItemDown && pItemDown->GetStrTransactionID().CompareNoCase (pItemCur->GetStrTransactionID()) == 0)
				bDownEnable = TRUE;
		}
		//
		// Can be moved up only if:
		// 1) nSelected > 0
		// 2) TransID [nSelected] = TransID [nSelected-1]
		if (pItemCur && nSelected > 0)
		{
			CaBaseTransactionItemData* pItemUp = (CaBaseTransactionItemData*)m_cListCtrl.GetItemData (nSelected - 1);
			if (pItemUp && pItemUp->GetStrTransactionID().CompareNoCase (pItemCur->GetStrTransactionID()) == 0)
				bUpEnable = TRUE;
		}
	}
	m_cButtonUp.EnableWindow  (bUpEnable);
	m_cButtonDown.EnableWindow(bDownEnable);

	int nSelCount = m_cListCtrl.GetSelectedCount();
	if (nSelCount == nCount && nCount != 0)
		CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	else
		CheckRadioButton (IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
}

void CxDlgRecover::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	EnableButtons();
	
	*pResult = 0;
}

void CxDlgRecover::OnRadioWholeTransaction() 
{
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		m_cListCtrl.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
	}
}

BOOL CxDlgRecover::PrepareParam (CaRecoverParam& param)
{
	CString strMsg;
	UpdateData (TRUE);

	param.m_bImpersonateUser = m_bImpersonateUser;
	param.m_bErrorOnAffectedRowCount = m_bErrorOnAffectedRowCount;
	param.m_bNoRule = m_bNoRule;
	param.m_bScanJournal = m_bCheckScanJournal;
	param.m_bWholeTransaction = (GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2) == IDC_RADIO1);
	param.m_pQueryTransactionInfo = m_pQueryTransactionInfo;
	IJA_QueryUserInCurSession (param.m_strConnectedUser);

	BOOL bNotValid = FALSE;
	BOOL bAsk2UndoRollbacked = FALSE;
	BOOL bAsk2UndoPartialNJ  = FALSE;
	BOOL bTableHasConstraint = FALSE;

	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
		{
			CaBaseTransactionItemData* pItem = (CaBaseTransactionItemData*)m_cListCtrl.GetItemData(i);
			TxType nType = pItem->GetAction();
			switch (nType)
			{
			case T_DELETE:
			case T_INSERT:
			case T_BEFOREUPDATE:
			case T_AFTERUPDATE:
				break;
			default:
				bNotValid = TRUE;
				break;
			}

			param.m_listTransaction.AddTail (pItem);
		}
	}

	BOOL bError = FALSE;
	CString strTrans;
	POSITION pos = param.m_listTransaction.GetHeadPosition();
	while (pos != NULL && !bError)
	{
		CaBaseTransactionItemData* Item = param.m_listTransaction.GetNext (pos);
		strTrans = Item->GetStrTransactionID();
		for (i=0; i<nCount; i++)
		{
			if (!(m_cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED))
			{
				CaBaseTransactionItemData* pItemNotSel = (CaBaseTransactionItemData*)m_cListCtrl.GetItemData(i);
				if (!pItemNotSel->IsBeginEnd())
				{
					if (strTrans.CompareNoCase (pItemNotSel->GetStrTransactionID()) == 0)
					{
						bError = TRUE;
						break;
					}
				}
			}
		}
		if (! Item->GetJournal())
			bAsk2UndoPartialNJ = TRUE;
		if (! Item->GetCommit())
			bAsk2UndoRollbacked = TRUE;
		if (m_bNoRule && !bTableHasConstraint)
			bTableHasConstraint = RCRD_IsTableHasConstraint(Item->GetTable(), Item->GetTableOwner(), m_listConstraint);
	}

	if (bError)
	{
		//
		// Are you sure you want to recover only parts of transactions, and lose transaction integrity ?
		strMsg.LoadString (IDS_RECOVER_PATIAL);

		int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
		if (nAnswer != IDYES)
			return FALSE;
	}

	if ((int)m_cListCtrl.GetItemCount() != (int)m_cListCtrl.GetSelectedCount())
	{
		//
		// You haven't selected the whole transactions corresponding to the initial selected lines. Continue?
		strMsg.LoadString (IDS_NOT_SELECT_ALLTRANS);

		int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
		if (nAnswer != IDYES)
			return FALSE;
	}
	
	if (bNotValid)
	{
		//
		// Only insert, update, and delete statements can be recovered.\n
		// The selection includes some statements of a different type, which will be skipped in the recover operation.\n\n Continue ?
		strMsg.LoadString (IDS_ONLY_INSxUPDxDELxALLOWED_RECOVER);

		if (AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OKCANCEL)!=IDOK )
			return FALSE;
	}
	if (bAsk2UndoPartialNJ ) {
		//
		// You are asking to recover transactions involving non-journaled tables.\n
		// The non-journaled part of such transactions will not be recovered.
		// \n\nContinue ?
		strMsg.LoadString (IDS_RECOVER_NON_JOURNAL_TRANS);

		if (AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OKCANCEL)!=IDOK )
			return FALSE;
	}
	if (bAsk2UndoRollbacked) {
		//
		// Changes of a transaction that has been rollbacked cannot be 'undone'.
		strMsg.LoadString (IDS_UNDO_ROLLBACK_TRANS);

		AfxMessageBox (strMsg, MB_ICONERROR|MB_OK);
		return FALSE;
	}

	if (bTableHasConstraint)
	{
		//
		// _T("You choose 'NoRule Option' and selected transactions to Recover contain table which has Constraint");
		CString strMsg;
		CString strRecover;
		strRecover.LoadString(IDS_RECOVER);
		strRecover.MakeLower();
		AfxFormatString1 (strMsg, IDS_REDOxRECOVER_NORULExCONSTRAINT, (LPCTSTR)strRecover);
		if (AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OKCANCEL)!=IDOK )
			return FALSE;
	}

	if (!RCRD_CheckOrder(_T("RECOVER"), m_cListCtrl, m_listTransaction))
		return FALSE;

	if (!RCRD_CheckValidateTransaction (param.m_listTransaction))
		return FALSE;
	return TRUE;
}

BOOL CxDlgRecover::IncludesCommitBetweenTransactions(CaRecoverParam& param)
{
	if (!param.m_bImpersonateUser)
		return FALSE;

	BOOL bResult = FALSE;
	CString csUser = _T("");
	POSITION pos = param.m_listTransaction.GetTailPosition();
	while (pos != NULL && !bResult) {
		CaBaseTransactionItemData* pBase = param.m_listTransaction.GetPrev (pos);
		if (!isUndoRedoable(pBase->GetAction()))
			continue;
		switch (pBase->GetAction()) {
			case T_DELETE:
			case T_INSERT:
			case T_AFTERUPDATE:
				if (pBase->GetUser().CompareNoCase(csUser)!=0) {
					if (csUser.IsEmpty())
						csUser = pBase->GetUser() ;
					else 
						bResult = TRUE;
				}
				break;
			default:
				break;
		}
	}
	return bResult;
}

BOOL CxDlgRecover::ScanToTheEnd(CaRecoverParam& param)
{
	CaQueryTransactionInfo QueryTransactionInfo(*m_pQueryTransactionInfo);
	QueryTransactionInfo.SetBeforeAfter (_T(""), _T(""));
	QueryTransactionInfo.SetUser(theApp.m_strAllUser);
	// TO BE FINISHED: for improving the speed, use the -start_lsn option as well

	CString strMsg;

	BOOL bContinueWithNJAfterMessage      = TRUE;
	BOOL bContinueWithChangedRowMessage   = TRUE;
	BOOL bContinueWithTableAlteredMessage = TRUE;

	BOOL bMissingJournalPartsAfter = FALSE;
	BOOL bRowsChangedAfterWards    = FALSE;
	BOOL bTablesOrRulesChanged     = FALSE;

	param.m_pSession->Release();
	CTypedPtrList < CObList, CaDatabaseTransactionItemData* > listMainTrans;
	BOOL bResult = IJA_QueryDatabaseTransaction (&QueryTransactionInfo, listMainTrans);
	while (bResult) {

		// update the "end of transaction" LSN long integer values in both lists for later comparisions
		unsigned long lsn_h,lsn_l;
		unsigned long ltran_high, ltran_low;

		// init with -1 -1 for double check that all were filled. Also init flags for displaying messages once per row to be undone
		POSITION posItem2Undo = param.m_listTransaction.GetHeadPosition();
		while (posItem2Undo != NULL)  {
			CaBaseTransactionItemData* p2undo = param.m_listTransaction.GetNext (posItem2Undo);
			p2undo->SetLsnEndPerValues(-1L, -1L);
			p2undo->SetMsgTidChanged(FALSE);
			p2undo->SetMsgTblChanged(FALSE);
		}

		POSITION pos = listMainTrans.GetHeadPosition();
		while (pos !=  NULL && bResult) {
			CaDatabaseTransactionItemData* pItem = listMainTrans.GetNext (pos);
			ASSERT (pItem);
			if (!pItem)
				continue;
			pItem->GetEndTransaction().GetLsn(lsn_h,lsn_l);
			pItem->GetEndTransaction().GetTransactionID(ltran_high,ltran_low);
			CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction = pItem->GetDetailTransaction();
			POSITION p1 = listTransaction.GetHeadPosition();
			while (p1 != NULL && bResult) {
				CaDatabaseTransactionItemDataDetail* pScannedItem = listTransaction.GetNext(p1);
				pScannedItem->SetLsnEndPerValues(lsn_h,lsn_l); // all items in the sub-list belong to the given transaction
			}
			// update End of transaction LSN long integers in the list of row changes to be undone
			posItem2Undo = param.m_listTransaction.GetHeadPosition();
			while (posItem2Undo != NULL)  {
				unsigned long lt_high, lt_low;
				CaBaseTransactionItemData* p2undo = param.m_listTransaction.GetNext (posItem2Undo);
				p2undo->GetTransactionID(lt_high, lt_low);
				if (lt_high == ltran_high && lt_low == ltran_low) {
					p2undo->SetLsnEndPerValues(lsn_h,lsn_l);
				}
				if ( bContinueWithNJAfterMessage &&
					! pItem ->GetJournal() && pItem->GetCommit() &&
					(ltran_high > lt_high || ( ltran_high == lt_high &&  ltran_low > lt_low ) )
					) {
					bContinueWithNJAfterMessage = FALSE;
					bMissingJournalPartsAfter   = TRUE;
					//
					// Transactions involving non-journaled tables were committed later than the transaction you are trying to recover\n
					// Cannot detect whether the rows involved in the transaction to be undone were changed afterwards, nor if the corresponding tables were altered afterwards
					// \n\nContinue ?
					strMsg.LoadString (IDS_SCANTOTHEEND_MSG1);

					int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
					if (nAnswer != IDYES) {
						bResult = FALSE;
						break;
					}
				}
			}
		}
		posItem2Undo = param.m_listTransaction.GetHeadPosition();
		while (posItem2Undo != NULL && bResult)  {
			CaBaseTransactionItemData* p2undo = param.m_listTransaction.GetNext (posItem2Undo);
				p2undo->GetLsnEndPerValues(lsn_h, lsn_l);
				if (lsn_h == (unsigned long) -1L && lsn_l == (unsigned long) -1L) {
					AfxMessageBox ("Internal Error : Cannot get End of Transaction LSN for row changes to be undone", MB_ICONSTOP);
					bResult = FALSE;
					break;
				}
		}

		pos = listMainTrans.GetHeadPosition();
		while (pos !=  NULL && bResult) {
			unsigned long l_scanned_transID_high, l_scanned_transID_low;
			CaDatabaseTransactionItemData* pItem = listMainTrans.GetNext (pos);
			ASSERT (pItem);
			if (!pItem)
				continue;
			if (!Get2LongsFromHexString(pItem->GetTransactionID(), l_scanned_transID_high, l_scanned_transID_low)) {
				AfxMessageBox ("Internal Error : Invalid Transaction ID format", MB_ICONSTOP);
				bResult = FALSE;
				break;
			}
			if (!pItem->GetCommit())
				continue;

			CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction = pItem->GetDetailTransaction();
			POSITION p1 = listTransaction.GetHeadPosition();
			while (p1 != NULL && bResult) {
				CaDatabaseTransactionItemDataDetail* pScannedItem = listTransaction.GetNext(p1);
				// we are on a given LSN . search for interference against all LSN's to be undone
				posItem2Undo = param.m_listTransaction.GetHeadPosition();
				while (posItem2Undo != NULL && bResult)  {
					CaBaseTransactionItemData* p2undo = param.m_listTransaction.GetNext (posItem2Undo);
					unsigned long l_transID2undo_high,l_transID2undo_low;
					p2undo->GetTransactionID(l_transID2undo_high,l_transID2undo_low);
					if (l_transID2undo_high == l_scanned_transID_high && l_transID2undo_low == l_scanned_transID_low)
						continue; // don't test against same transaction ...
					switch (p2undo->GetAction()) {
						case T_INSERT:
						case T_AFTERUPDATE:
							// test whether for a later LSN, the data of the given TID and table  was changed
							if (p2undo->GetTid() == pScannedItem->GetTid() && pScannedItem->isLSNEndhigher(p2undo) &&
								p2undo->GetTable().Compare((LPCTSTR )pScannedItem->GetTable())==0 &&
								p2undo->GetTableOwner().Compare((LPCTSTR )pScannedItem->GetTableOwner())==0 &&
								(pScannedItem->GetAction() == T_DELETE || pScannedItem->GetAction()== T_BEFOREUPDATE) &&
								!p2undo->HasMsgTidChangedBeenDisplayed() // don't display more than once for same exact row to be undone
							   ) {
								// look if scanned one not in the list of row changes to be undone;
								BOOL bInList2BeUndone = FALSE;
								POSITION postemp = posItem2Undo;
								while (postemp != NULL && bResult)  {
									CaBaseTransactionItemData* ptmp = param.m_listTransaction.GetNext (postemp);
									unsigned long lsnh,lsnl;
									unsigned long scanned_lsnh,scanned_lsnl;
									ptmp->GetLsn(lsnh,lsnl);
									pScannedItem->GetLsn(scanned_lsnh,scanned_lsnl);
									if (lsnh == scanned_lsnh && lsnl == scanned_lsnl) {
										bInList2BeUndone = TRUE;
										break;
									}
								}
								if (bInList2BeUndone) /* already in list. Don't provide message for this one */
									break;
								bRowsChangedAfterWards = TRUE;
								if ( bContinueWithChangedRowMessage) {
									p2undo->SetMsgTidChanged(TRUE);
									LPCTSTR paction = pScannedItem->GetAction() == T_DELETE ? m_strActionDelete: m_strActionUpdate;
									CString strFormat;
									//
									// row [ %s ] (TID %ld)\n
									// from table %s.%s\n
									// has already been %s later than the transaction you are trying to recover\n\n
									// Continue ? 
									strFormat.LoadString(IDS_SCANTOTHEEND_MSG2);

									strMsg.Format(strFormat,
										(LPCTSTR) p2undo->GetData(),
										p2undo->GetTid(),
										(LPCTSTR)p2undo->GetTableOwner(),
										(LPCTSTR)p2undo->GetTable(),
										paction);
									int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
									if (nAnswer != IDYES) {
										bResult = FALSE;
										break;
									}
								}
							}
							break;
						case T_DELETE:
							break; // results in going ahead with other tests (wheter the table was altered, etc.. 
						default:
							continue; // only T_INSERT T_AFTERUPDATE T_DELETE should go ahead with additional tests
					}
					// test whether the table was altered
					if ( pScannedItem->isLSNEndhigher(p2undo) &&
						p2undo->GetTable().Compare((LPCTSTR )pScannedItem->GetTable())==0 &&
						p2undo->GetTableOwner().Compare((LPCTSTR )pScannedItem->GetTableOwner())==0 &&
						(pScannedItem->GetAction() == T_ALTERTABLE || pScannedItem->GetAction()== T_DROPTABLE) &&
						! p2undo->HasMsgTblChangedBeenDisplayed() // don't display more than once for a givne row change to be undone
					   ) {
						bTablesOrRulesChanged = TRUE;
						if ( bContinueWithTableAlteredMessage) {
							p2undo->SetMsgTblChanged(TRUE);
							LPCTSTR paction = pScannedItem->GetAction() == T_ALTERTABLE ? m_strActionAlter: m_strActionDrop;
							LPCTSTR pmore   = pScannedItem->GetAction() == T_ALTERTABLE ? m_strActionMore:CString("");
							CString strFormat;
							//
							// row [ %s ] (TID %ld)\n
							// belongs to table %s.%s, which has been %s later than the transaction you are trying to recover%s\n\n
							// Continue ? 
							strFormat.LoadString(IDS_SCANTOTHEEND_MSG3);

							strMsg.Format(strFormat,
								(LPCTSTR) p2undo->GetData(),
								p2undo->GetTid(),
								(LPCTSTR)p2undo->GetTableOwner(),
								(LPCTSTR)p2undo->GetTable(),
								paction,
								pmore);
							int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
							if (nAnswer != IDYES) {
								bResult = FALSE;
								break;
							}
						}
					}
				}
			}
		}
		break;
	}
	param.m_pSession->Open();

		while (!listMainTrans.IsEmpty())
			delete listMainTrans.RemoveHead();

	return bResult;
}

void CxDlgRecover::OnButtonRecoverNow() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CaRecoverParam param;
		ASSERT (m_pQueryTransactionInfo);
		if (!m_pQueryTransactionInfo)
			return;
		CString strMsg;
		CString strDatabase, strDatabaseOwner;
		m_pQueryTransactionInfo->GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session(m_pQueryTransactionInfo->GetNode(), strDatabase);
		param.SetSession (&session);

		if (!PrepareParam (param))
			return;
		if (param.m_bScanJournal) {
			if (!ScanToTheEnd(param))
				return;
		}
		if (IncludesCommitBetweenTransactions(param)) {
			//
			// All Transactions to be recovered were NOT executed by the same user.\n
			// You have asked to impersonate the initial transaction users.\n
			// As a result, the executed script will include COMMIT statements between the transactions to be undone.\n
			// In case of failure of one of them, automatic recovery cannot be guaranteed\n\nContinue ?
			strMsg.LoadString(IDS_RECOVERNOW_MSG1);

			int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
			if (nAnswer != IDYES)
				return;
		}
		CString csErrMsg;
		if (!param.Recover(csErrMsg))
		{
			CString strFormat;
			//
			// Cannot Recover the Statements/Transactions.\n\n%s
			strFormat.LoadString(IDS_RECOVERNOW_MSG2);
			strMsg.Format(strFormat,(LPCTSTR)csErrMsg);
			AfxMessageBox (strMsg);
			return;
		}
		session.Release ();
		//
		// The Recover (undo) operation has completed successfully.
		strMsg.LoadString(IDS_RECOVER_SUCCESSFUL);

		AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
		EndDialog (IDOK);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		TRACE0 ("Raise exception: CxDlgRecover::OnButtonRecoverNow() \n");
	}
}

void CxDlgRecover::OnButtonGenerateScript() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CaRecoverParam param;
		ASSERT (m_pQueryTransactionInfo);
		if (!m_pQueryTransactionInfo)
			return;
		CString strMsg;
		CString strDatabase, strDatabaseOwner;
		m_pQueryTransactionInfo->GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session(m_pQueryTransactionInfo->GetNode(), strDatabase);
		param.SetSession (&session);

		if (!PrepareParam (param))
			return;
		if (param.m_bScanJournal) {
			if (!ScanToTheEnd(param))
				return;
		}
		if (IncludesCommitBetweenTransactions(param)) {
			//
			// All Transactions to be recovered were NOT executed by the same user.\n
			// You have asked to impersonate the initial transaction users.\n
			// As a result, the generated script will include COMMIT statements between the transactions to be undone, 
			// which may be problematic in case of failure of one of them.\n\nContinue ?
			strMsg.LoadString(IDS_RECOVER_GENSCRIPT_MSG1);

			int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
			if (nAnswer != IDYES)
				return;
		}
		CString csErrMsg;
		BOOL bRoundingErrorsPossibleInText = FALSE;
		if (param.GenerateScript(csErrMsg, bRoundingErrorsPossibleInText))
		{
			if (param.m_strScript.Find(COL_DISP_UNAVAILABLE)!=-1) {
				//
				// The involved data include data of a type that can't be displayed in a text script.\nThe script cannot be generated.
				strMsg.LoadString(IDS_RECOVER_GENSCRIPT_MSG2);

				AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
				return;
			}
			if (bRoundingErrorsPossibleInText) {
				//
				// The script contains a text representation of some data types that may lead to rounding errors, 
				// such as "float" or "decimal" data types.\n\n
				// Continue anyhow ?
				// \n\n[ Note that the "recover now" functionality doesn't have this problem ]
				strMsg.LoadString(IDS_RECOVER_GENSCRIPT_MSG3);

				int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
				if (nAnswer != IDYES)
					return;
			}
			CxScript dlg;
			dlg.m_strNode = m_pQueryTransactionInfo->GetNode();
			dlg.m_strDatabase = strDatabase;
			dlg.m_strTitle.LoadString (IDS_RECOVER_SCRIPT);
			dlg.m_strEdit1 = param.m_strScript;
			dlg.m_bErrorOnAffectedRowCount = m_bErrorOnAffectedRowCount;
			dlg.m_nHelpID = ID_HELP_RECOVER_SCRIPT;
			dlg.DoModal();
		}
		else
		{
			CString strFormat;
			//
			// Cannot Generate the External Script.\n\n%s
			strFormat.LoadString(IDS_RECOVER_GENSCRIPT_MSG4);

			strMsg.Format(strFormat, (LPCTSTR)csErrMsg);
			AfxMessageBox (strMsg);
		}
		session.Release();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
	}
	catch(...)
	{
		TRACE0 ("CxDlgRecover::OnButtonGenerateScript() \n");
	}
}


void CxDlgRecover::OnButtonRecoverNowScript() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CaRecoverParam param;
		ASSERT (m_pQueryTransactionInfo);
		if (!m_pQueryTransactionInfo)
			return;
		CString strMsg;
		CString strDatabase, strDatabaseOwner;
		m_pQueryTransactionInfo->GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session(m_pQueryTransactionInfo->GetNode(), strDatabase);
		param.SetSession (&session);

		if (!PrepareParam (param))
			return;
		if (param.m_bScanJournal) {
			if (!ScanToTheEnd(param))
				return;
		}
		if (IncludesCommitBetweenTransactions(param)) {
			//
			// All Transactions to be recovered were NOT executed by the same user.\n
			// You have asked to impersonate the initial transaction users.\n
			// As a result, the executed script will include COMMIT statements between the transactions to be undone.\n
			// In case of failure of one of them, automatic recovery cannot be guaranteed\n\nContinue ?
			strMsg.LoadString(IDS_RECOVERNOW_MSG1);

			int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
			if (nAnswer != IDYES)
				return;
		}
		CString csErrMsg;
		if (param.RecoverNowScript(csErrMsg))
		{
			CxScript dlg;
			dlg.m_bReadOnly = TRUE;
			dlg.m_strTitle.LoadString (IDS_RECOVERNOW_SCRIPT);
			dlg.m_strEdit1 = param.m_strScript;
			dlg.m_bErrorOnAffectedRowCount = m_bErrorOnAffectedRowCount;
			dlg.m_nHelpID = ID_HELP_RECOVERNOW_SCRIPT;
			dlg.DoModal();
		}
		else
		{
			CString strFormat;
			//
			// Cannot generate the "Recover Now" script.\n\n%s
			strFormat.LoadString(IDS_RECOVERNOW_SCRIPT_MSG1);
			strMsg.Format(strFormat, (LPCTSTR)csErrMsg);
			AfxMessageBox (strMsg);
		}
		session.Release();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
	}
	catch(...)
	{
		TRACE0 ("CxDlgRecover::OnButtonRecoverNowScript() \n");
	}
}


void CxDlgRecover::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	RCRD_SelchangeTabRuleConstraint(m_pDlgListRules, m_pDlgListIntegrities, m_pDlgListConstraints, m_pCurrentPage, &m_cTab1);
}

void CxDlgRecover::OnDestroy() 
{
	CDialog::OnDestroy();
	//
	// Cleanup the list of rules and constraints:
	while (!m_listRule.IsEmpty())
		delete m_listRule.RemoveTail();
	while (!m_listConstraint.IsEmpty())
		delete m_listConstraint.RemoveTail();
	while (!m_listIntegrity.IsEmpty())
		delete m_listIntegrity.RemoveTail();
}

long CxDlgRecover::OnDisplayProperty (WPARAM wParam, LPARAM lParam)
{
	int nSel = m_cTab1.GetCurSel();
	return RCRD_OnDisplayProperty (this, m_pPropFrame, nSel, wParam, lParam);
}

BOOL CxDlgRecover::PreTranslateMessage(MSG* pMsg) 
{
	//
	// Let the ToolTip process this message.
	m_toolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CxDlgRecover::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo (m_hWnd, IDD_XRECOVER);
	return TRUE;
}
