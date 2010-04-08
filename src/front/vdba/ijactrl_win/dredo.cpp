/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dredo.cpp, Implementation File 
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of Redo Dialog Box
**
** History:
**
** 29-Dec-1999 (uk$so01)
**    created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 15-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
**/


#include "stdafx.h"
#include "ijactrl.h"
#include "dredo.h"
#include "ijadmlll.h"
#include "ijactdml.h"
#include "rcrdtool.h"
#include "xscript.h"
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


CxDlgRedo::CxDlgRedo(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRedo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgRedo)
	m_bNoRule = FALSE;
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_bImpersonateUser = FALSE;
	m_bErrorOnAffectedRowCount = TRUE;
	//}}AFX_DATA_INIT
	m_pQueryTransactionInfo = NULL;

	m_pDlgListRules = NULL;
	m_pDlgListIntegrities = NULL;
	m_pDlgListConstraints = NULL;
	m_pCurrentPage = NULL;
	m_pPropFrame = NULL;
}


void CxDlgRedo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgRedo)
	DDX_Control(pDX, IDC_BUTTON6, m_cButtonInitialOrder);
	DDX_Control(pDX, IDC_BUTTON5, m_cButtonInitialSelection);
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonDown);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonUp);
	DDX_Control(pDX, IDC_COMBO2, m_cComboDatabase);
	DDX_Control(pDX, IDC_COMBO1, m_cComboNode);
	DDX_Check(pDX, IDC_CHECK1, m_bNoRule);
	DDX_CBString(pDX, IDC_COMBO1, m_strNode);
	DDX_CBString(pDX, IDC_COMBO2, m_strDatabase);
	DDX_Check(pDX, IDC_CHECK7, m_bImpersonateUser);
	DDX_Check(pDX, IDC_CHECK8, m_bErrorOnAffectedRowCount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgRedo, CDialog)
	//{{AFX_MSG_MAP(CxDlgRedo)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboNode)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonDown)
	ON_BN_CLICKED(IDC_BUTTON5, OnButtonInitialSelection)
	ON_BN_CLICKED(IDC_BUTTON6, OnButtonInitialOrder)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_BN_CLICKED(IDOK, OnButtonRedoNow)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonGenerateScript)
	ON_BN_CLICKED(IDC_RADIO1, OnRadioWholeTransaction)
	ON_BN_CLICKED(IDC_BUTTON9, OnButtonRedoNowScript)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LISTCTRLITEM_PROPERTY, OnDisplayProperty)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRedo message handlers

BOOL CxDlgRedo::OnInitDialog() 
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

	try
	{
		FillComboNode();
		RCRD_DisplayTransaction(m_cListCtrl, &m_listTransaction);
		RCRD_SelectItems (m_cListCtrl, m_pArrayItem, m_nArraySize);
		EnableButtons();
#if !defined (SIMULATION)
		SORTPARAMS sp;
		sp.m_bAsc = TRUE;
		m_cListCtrl.SortItems(CompareSubItemInitialOrder, (LPARAM)&sp);
#endif
		//
		// Preselect node & database:
		CString strNode = m_pQueryTransactionInfo->GetNode();
		CString strDatabase, strDatabaseOwner;
		m_pQueryTransactionInfo->GetDatabase(strDatabase, strDatabaseOwner);
		int nSel = m_cComboNode.FindStringExact (-1, strNode);
		if (nSel != CB_ERR)
			m_cComboNode.SetCurSel (nSel);
		OnSelchangeComboNode();
		nSel = m_cComboDatabase.FindStringExact (-1, strDatabase);
		if (nSel != CB_ERR)
			m_cComboDatabase.SetCurSel (nSel);

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
		m_toolTip.AddTool(&m_cButtonUp,              strTools);//_T("Move Item Up")
		strTools.LoadString(IDS_TOOLSTIP_BUTTON_SEL);
		m_toolTip.AddTool(&m_cButtonInitialSelection,strTools);//_T("Default Selection")
		strTools.LoadString(IDS_TOOLSTIP_BUTTON_ORDER);
		m_toolTip.AddTool(&m_cButtonInitialOrder,    strTools);//_T("Default Order")
	}
	catch (...)
	{
		TRACE0 ("Raise exception at: CxDlgRedo::OnInitDialog()\n");
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CxDlgRedo::OnSelchangeComboNode() 
{
	CWaitCursor doWaitCursor;
	try
	{
		int nSel = m_cComboNode.GetCurSel();
		if (nSel == CB_ERR)
			return;
		m_cComboDatabase.ResetContent();
		CString strNode;
		m_cComboNode.GetLBText (nSel, strNode);
		FillComboDatabase (strNode);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch (...)
	{
		AfxMessageBox (_T("System error, CxDlgRedo::OnSelchangeComboNode, query database failed"));
	}
}


void CxDlgRedo::FillComboNode()
{
	m_cComboDatabase.ResetContent();
	m_cComboNode.ResetContent();
	CStringList& ln = theApp.m_listNode;
	CString strItem;
	POSITION  pos = ln.GetHeadPosition();
	while (pos != NULL)
	{
		strItem = ln.GetNext (pos);
		m_cComboNode.AddString(strItem);
	}
}

void CxDlgRedo::FillComboDatabase(LPCTSTR lpszNode)
{
	CTypedPtrList<CObList, CaDatabase*> listDatabase;
	if (IJA_QueryDatabase (lpszNode, listDatabase))
	{
		POSITION pos = listDatabase.GetHeadPosition();
		while (pos != NULL)
		{
			CaDatabase* pDb = listDatabase.GetNext(pos);
			m_cComboDatabase.AddString(pDb->GetItem());
			delete pDb;
		}
	}
}


void CxDlgRedo::OnButtonUp() 
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
		TRACE0 ("Raise exception at: CxDlgRedo::OnButtonUp()\n");
	}
}

void CxDlgRedo::OnButtonDown() 
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
		TRACE0 ("Raise exception at: CxDlgRedo::OnButtonDown()\n");
	}
}

void CxDlgRedo::OnButtonInitialSelection() 
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
		TRACE0 ("CxDlgRedo::OnButtonInitialSelection() \n");
	}
}

void CxDlgRedo::OnButtonInitialOrder() 
{
	RCRD_InitialOrder(m_cListCtrl, &m_listTransaction);
	EnableButtons();
}



void CxDlgRedo::EnableButtons()
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

void CxDlgRedo::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	EnableButtons();
	*pResult = 0;
}

BOOL CxDlgRedo::PrepareParam (CaRedoParam& param)
{
	UpdateData (TRUE);
	CString strMsg;

	param.m_bImpersonateUser = m_bImpersonateUser;
	param.m_bErrorOnAffectedRowCount = m_bErrorOnAffectedRowCount;
	param.m_bNoRule = m_bNoRule;
	param.m_strNode = m_strNode;
	param.m_strDatabase = m_strDatabase;
	param.m_bWholeTransaction = (GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO2) == IDC_RADIO1);
	param.m_pQueryTransactionInfo = m_pQueryTransactionInfo;
	IJA_QueryUserInCurSession (param.m_strConnectedUser);

	BOOL bNotValid = FALSE;
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

		if (m_bNoRule && !bTableHasConstraint)
			bTableHasConstraint = RCRD_IsTableHasConstraint(Item->GetTable(), Item->GetTableOwner(), m_listConstraint);
	}

	if (bError)
	{
		//
		// Are you sure you want to redo only parts of transactions, and lose transaction integrity ?
		strMsg.LoadString(IDS_REDO_PATIAL);

		int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
		if (nAnswer != IDYES)
			return FALSE;
	}

	if ((int)m_cListCtrl.GetItemCount() != (int)m_cListCtrl.GetSelectedCount())
	{
		//
		// You haven't selected the whole transactions corresponding to the initial selected lines. Continue ?
		strMsg.LoadString(IDS_NOT_SELECT_ALLTRANS);

		int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
		if (nAnswer != IDYES)
			return FALSE;
	}

	if (bNotValid)
	{
		//
		// Only insert, update, and delete statements can be redone.\n
		// The selection includes some statements of a different type, which will be skipped in the redo operation.\n\n Continue ?
		strMsg.LoadString(IDS_ONLY_INSxUPDxDELxALLOWED_REDO);

		if (AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OKCANCEL)!=IDOK )
			return FALSE;
	}

	if (bTableHasConstraint)
	{
		//
		// _T("You choose 'NoRule Option' and selected transactions to Redo contain table which has Constraint");
		CString strMsg;
		CString strRedo;
		strRedo.LoadString(IDS_REDO);
		strRedo.MakeLower();
		AfxFormatString1 (strMsg, IDS_REDOxRECOVER_NORULExCONSTRAINT, (LPCTSTR)strRedo);
		if (AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OKCANCEL)!=IDOK )
			return FALSE;
	}

	if (!RCRD_CheckOrder(_T("REDO"), m_cListCtrl, m_listTransaction))
		return FALSE;

	if (!RCRD_CheckValidateTransaction (param.m_listTransaction))
		return FALSE;
	return TRUE;
}

BOOL CxDlgRedo::IncludesCommitBetweenTransactions(CaRedoParam& param)
{
	if (!param.m_bImpersonateUser)
		return FALSE;

	BOOL bResult = FALSE;
	CString csUser = _T("");
	POSITION pos = param.m_listTransaction.GetHeadPosition();
	while (pos != NULL && !bResult) {
		CaBaseTransactionItemData* pBase = param.m_listTransaction.GetNext (pos);
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

void CxDlgRedo::OnButtonRedoNow() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CaRedoParam param;

		CString strMsg;
		CString strDatabase, strDatabaseOwner;
		m_pQueryTransactionInfo->GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session(m_pQueryTransactionInfo->GetNode(), strDatabase); 
		param.SetSession (&session);

		if (!PrepareParam (param))
			return;

		if (m_pQueryTransactionInfo->GetNode().CompareNoCase (m_strNode) != 0 || strDatabase.CompareNoCase (m_strDatabase) != 0)
		{
			// 
			// 'Redoing' statements on a different node or database can be done only through the 'Generate External Script' Dialog.\n\nLaunch that dialog ?
			strMsg.LoadString(IDS_REDONOW_MSG1);

			if (AfxMessageBox (strMsg, MB_OKCANCEL|MB_ICONEXCLAMATION)== IDOK)
				OnButtonGenerateScript();
		}
		else
		{
			if (IncludesCommitBetweenTransactions(param)) {
				//
				// All Transactions to be redone were NOT executed by the same user.\n
				// You have asked to impersonate the initial transaction users.\n
				// As a result, the executed script will include COMMIT statements between the transactions to be redone.\n
				// In case of failure of one of them, the undo of the previously committed ones is not guaranteed.\n\nContinue ?
				strMsg.LoadString (IDS_REDONOW_MSG2);

				int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
				if (nAnswer != IDYES)
					return;
			}

			CString csErrMsg;
			if (!param.Redo(csErrMsg))
			{
				CString strFormat;
				//
				// Cannot Redo the Statements/Transactions.\n\n%s
				strFormat.LoadString(IDS_REDONOW_MSG3);

				strMsg.Format(strFormat, (LPCTSTR)csErrMsg);
				AfxMessageBox (strMsg);
				return;
			}
			session.Release();
			//
			// The Redo operation has completed successfully.
			strMsg.LoadString(IDS_REDONOW_SUCCESSFUL);

			AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
			EndDialog (IDOK);
		}

		session.Release();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		TRACE0 ("CxDlgRedo::OnButtonRedoNow() \n");
	}
}

void CxDlgRedo::OnButtonGenerateScript() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CaRedoParam param;
		CString strMsg;
		CString strDatabase, strDatabaseOwner;
		m_pQueryTransactionInfo->GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session(m_pQueryTransactionInfo->GetNode(),strDatabase ); 
		param.SetSession (&session);

		if (!PrepareParam (param))
			return;

		if (IncludesCommitBetweenTransactions(param)) {
			//
			// All Transactions to be redone were NOT executed by the same user.\n
			// You have asked to impersonate the initial transaction users.\n
			// As a result, the generated script will include COMMIT statements between the transactions to be redone,\n
			// which may be problematic in case of failure of one of them.\n\nContinue ?

			strMsg.LoadString(IDS_REDO_GENSCRIPT_MSG1);
			int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
			if (nAnswer != IDYES)
				return;
		}

		CString csErrMsg;
		BOOL bRoundingErrorsPossibleInText = FALSE;
		if (param.GenerateScript(csErrMsg,bRoundingErrorsPossibleInText))
		{
			if (param.m_strScript.Find(COL_DISP_UNAVAILABLE)!=-1) {
				// 
				// The involved data include data of a type that can't be displayed in a text script.\nThe script cannot be generated.
				strMsg.LoadString(IDS_REDO_GENSCRIPT_MSG2);

				AfxMessageBox (strMsg, MB_ICONSTOP|MB_OK);
				return;
			}
			if (bRoundingErrorsPossibleInText) {
				//
				// The script contains a text representation of some data types that may lead to rounding errors, 
				// such as "float" or "decimal" data types.\n\n
				// Continue anyhow ?\n\n[ Note that the "redo now" functionality doesn't have this problem (but works only on the same node/database as the original transactions) ]
				strMsg.LoadString(IDS_REDO_GENSCRIPT_MSG3);

				int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
				if (nAnswer != IDYES)
					return;
			}
			CxScript dlg;
			dlg.m_strNode = param.m_strNode;
			dlg.m_strDatabase = param.m_strDatabase;
			dlg.m_strTitle.LoadString (IDS_REDO_SCRIPT);
			dlg.m_strEdit1 = param.m_strScript;
			dlg.m_bCall4Redo = TRUE;
			dlg.m_bErrorOnAffectedRowCount = m_bErrorOnAffectedRowCount;
			dlg.m_nHelpID = ID_HELP_REDO_SCRIPT;
			dlg.DoModal();

			session.Activate();
		}
		else
		{
			CString strFormat;
		
			//
			// Cannot generate the External Script.\n\n%s
			strFormat.LoadString(IDS_REDO_GENSCRIPT_MSG4);

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
		TRACE0 ("CxDlgRedo::OnButtonGenerateScript() \n");
	}
}

void CxDlgRedo::OnRadioWholeTransaction() 
{
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		m_cListCtrl.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CxDlgRedo::OnButtonRedoNowScript() 
{
	try
	{
		CWaitCursor doWaitCursor;
		CaRedoParam param;
		CString strMsg;
		CString strDatabase, strDatabaseOwner;
		m_pQueryTransactionInfo->GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session(m_pQueryTransactionInfo->GetNode(), strDatabase); 
		param.SetSession (&session);

		if (!PrepareParam (param))
			return;

		if (IncludesCommitBetweenTransactions(param)) {
			//
			// All Transactions to be redone were NOT executed by the same user.\n
			// You have asked to impersonate the initial transaction users.\n
			// As a result, the generated script will include COMMIT statements between the transactions to be redone,\n
			// which may be problematic in case of failure of one of them.\n\nContinue ?
			strMsg.LoadString(IDS_REDO_GENSCRIPT_MSG1);

			int nAnswer = AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_YESNO);
			if (nAnswer != IDYES)
				return;
		}

		CString csErrMsg;
		if (param.RedoNowScript(csErrMsg))
		{
			CxScript dlg;
			dlg.m_bReadOnly = TRUE;
			dlg.m_bCall4Redo = TRUE;
			dlg.m_strTitle.LoadString (IDS_REDONOW_SCRIPT);
			dlg.m_strEdit1 = param.m_strScript;
			dlg.m_bErrorOnAffectedRowCount = m_bErrorOnAffectedRowCount;
			dlg.m_nHelpID = ID_HELP_REDODOW_SCRIPT;
			dlg.DoModal();
		}
		else
		{
			CString strFormat;
			//
			// Cannot generate the "Redo Now" script.\n\n%s
			strFormat.LoadString(IDS_REDONOW_SCRIPT_MSG1);

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
		TRACE0 ("CxDlgRedo::OnButtonRedoNowScript() \n");
	}
}



void CxDlgRedo::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	RCRD_SelchangeTabRuleConstraint(m_pDlgListRules, m_pDlgListIntegrities, m_pDlgListConstraints, m_pCurrentPage, &m_cTab1);
}

void CxDlgRedo::OnDestroy() 
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


long CxDlgRedo::OnDisplayProperty (WPARAM wParam, LPARAM lParam)
{
	int nSel = m_cTab1.GetCurSel();
	return RCRD_OnDisplayProperty (this, m_pPropFrame, nSel, wParam, lParam);
}

BOOL CxDlgRedo::PreTranslateMessage(MSG* pMsg) 
{
	//
	// Let the ToolTip process this message.
	m_toolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CxDlgRedo::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo (m_hWnd, IDD_XREDO);
	return TRUE;
}
