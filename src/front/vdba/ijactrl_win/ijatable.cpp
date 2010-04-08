/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijatable.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modeless Dialog represents the Table Pane
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 15-Jun-2001 (uk$so01)
**    BUG #105009
**    Refresh the transactions depending the value of (abort/commit) checkbox
** 02-Jul-2001 (hanje04)
**    Removed static in defn of CompareSubItem as it conflicts with denf in 
**    sqlselec.h
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 02-Dec-2003 (schph01)
**    SIR #111389 Implement new button for displayed the checkpoint list.
** 13-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should be disconnected
**    if they ere not used.
** 22-Nov-2004 (schph01)
**    BUG #113511 replace CompareNoCase function by CollateNoCase
**    according to the LC_COLLATE category setting of the locale code
**    page currently in use.
** 01-Dec-2004 (uk$so01)
**    IJA BUG #113551
**    Ija should internally specify the fixed date format expected by auditdb 
**    command independently of the LOCALE setting.
**/

#include "stdafx.h"
#include "ijactrl.h"
#include "ijatable.h"
#include "ijactdml.h"
#include "rcrdtool.h"
#include "drecover.h"
#include "dredo.h"
#include "ijadmlll.h"
#include "sqlselec.h"
#include "ckplst.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const int nHeader = 5;
typedef struct tagSORTTABLECOLUMN
{
	SORTPARAMS m_sortParam;
	CTypedPtrList<CObList, CaColumn*>* m_pListColumn;

} SORTTABLECOLUMN;

enum 
{
	IDCT_AFTER = 1,
	IDCT_BEFOR,
	IDCT_CHECK_CHECKPOINT,
	IDCT_EDIT_CHECKPOINT,
	IDCT_ABORT_TRANS,
	IDCT_COMMIT_TRANS,
	IDCT_USER,
	IDCT_WAIT
};

static CaColumn* GetNthColumn (CTypedPtrList<CObList, CaColumn*>* pListColumn, int nItem);
static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

static BOOL ExistTransaction (CaDatabaseTransactionItemDataDetail* pDetail, CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr)
{
	POSITION pos = ltr.GetHeadPosition();
	unsigned long l1H, l1L;
	unsigned long l2H, l2L;
	pDetail->GetTransactionID (l1H, l1L);

	while (pos != NULL)
	{
		CaBaseTransactionItemData* pObj = ltr.GetNext (pos);
		pObj->GetTransactionID (l2H, l2L);
		
		if (l1H == l2H && l1L == l2L)
			return TRUE;
	}
	return FALSE;
}

CuDlgIjaTable::CuDlgIjaTable(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIjaTable::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIjaTable)
	m_bAfterCheckPoint = FALSE;
	m_bWait = FALSE;
	m_strUser = theApp.m_strAllUser;
	m_strCheckPointNo = _T("1");
	m_bAbortedTransaction = TRUE;
	m_bCommittedTransaction = TRUE;
	m_timeBefore = CTime::GetCurrentTime();
	m_timeAfter = CTime::GetCurrentTime();
	//}}AFX_DATA_INIT
	m_bInconsistent = FALSE;
	m_bColumnHeaderOK = FALSE;
	m_nColumnHeaderCount = 0;
	m_bEnableRecoverRedo = TRUE;

	m_sortparam.m_bAsc  = TRUE;
	m_sortparam.m_nItem = -1;
	m_bViewButtonClicked = FALSE;
	m_strAfter = _T("");
	m_strBefore = _T("");
	m_strInitialUser = theApp.m_strAllUser;
}


void CuDlgIjaTable::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIjaTable)
	DDX_Control(pDX, IDC_BUTTON_CHECKPOINT_LIST, m_ButtonCheckpointList);
	DDX_Control(pDX, IDC_CHECK2, m_cCheckAfterCheckPoint);
	DDX_Control(pDX, IDC_EDIT3, m_cEditAfterCheckPoint);
	DDX_Control(pDX, IDC_DATETIMEPICKER2, m_cDatePickerBefore);
	DDX_Control(pDX, IDC_DATETIMEPICKER1, m_cDatePickerAfter);
	DDX_Control(pDX, IDC_BUTTON4, m_cButtonSelectAll);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonRedo);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonRecover);
	DDX_Control(pDX, IDC_COMBO1, m_cComboUser);
	DDX_Check(pDX, IDC_CHECK2, m_bAfterCheckPoint);
	DDX_Check(pDX, IDC_CHECK3, m_bWait);
	DDX_CBString(pDX, IDC_COMBO1, m_strUser);
	DDX_Text(pDX, IDC_EDIT3, m_strCheckPointNo);
	DDX_Check(pDX, IDC_CHECK4, m_bAbortedTransaction);
	DDX_Check(pDX, IDC_CHECK5, m_bCommittedTransaction);
	//}}AFX_DATA_MAP

	//DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER1, m_timeAfter);
	//DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER2, m_timeBefore);
}


BEGIN_MESSAGE_MAP(CuDlgIjaTable, CDialog)
	//{{AFX_MSG_MAP(CuDlgIjaTable)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonView)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonRecover)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonRedo)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON4, OnButtonSelectAll)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_BN_CLICKED(IDC_BUTTON7, OnButtonMoveLog2Journals)
	ON_BN_CLICKED(IDC_CHECK2, OnCheckAfterCheckPoint)
	ON_BN_CLICKED(IDC_CHECK4, OnCheckAbortedTransaction)
	ON_BN_CLICKED(IDC_CHECK5, OnCheckCommittedTransaction)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboUser)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER2, OnDatetimechangeBefore)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER1, OnDatetimechangeAfter)
	ON_EN_CHANGE(IDC_EDIT3, OnChangeEditAfterCheckPoint)
	ON_BN_CLICKED(IDC_CHECK3, OnCheckWait)
	ON_EN_KILLFOCUS(IDC_EDIT3, OnKillfocusEditAfterCheckPoint)
	ON_BN_CLICKED(IDC_BUTTON_CHECKPOINT_LIST, OnButtonCheckpointList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIjaTable message handlers

BOOL CuDlgIjaTable::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ImageList.Create (IDB_IJAIMAGELIST2, 16, 0, RGB(255,0,255));
	m_ImageListOrder.Create (IDB_HEADERORDER, 16, 0, RGB(255,0,255));
	
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl.m_hWnd, GWL_STYLE, style);
	m_cListCtrl.SetFullRowSelected (TRUE);
	m_cListCtrl.SetLineSeperator(FALSE);
	m_cListCtrl.SetImageList  (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.ActivateColorItem();

	int nMoreHeader = 0;
	#if defined(SIMULATION)
	nMoreHeader = 1;
	#endif

	LVCOLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[nHeader+1] =
	{
		{IDS_HEADER_TRANSACTION,145, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_LSN,        145, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_COMMITDATE, 110, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_USER,        65, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_OPERATION,   80, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_NAME,       300, LVCFMT_LEFT, FALSE},
	};

	CString strHeader;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<(nHeader + nMoreHeader); i++)
	{
		strHeader.LoadString (lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn); 
	}

	//
	// Store the initial sizes of some controls:
	m_cListCtrl.GetWindowRect (m_rcInitialListCtrl);
	ScreenToClient (m_rcInitialListCtrl);
	m_sizeMargin.cx = m_rcInitialListCtrl.left;
	m_sizeMargin.cy = m_rcInitialListCtrl.left;
	m_cButtonRecover.GetWindowRect (m_rcInitialButtonRecover);
	ScreenToClient (m_rcInitialButtonRecover);
	m_cButtonRedo.GetWindowRect (m_rcInitialButtonRedo);
	ScreenToClient (m_rcInitialButtonRedo);
	EnableButtons();
	EnableCheckPoint();

	m_cDatePickerAfter.SetFormat (_T("dd'-'MMM'-'yyy':'HH':'mm':'ss"));
	m_cDatePickerBefore.SetFormat(_T("dd'-'MMM'-'yyy':'HH':'mm':'ss"));
	m_cDatePickerAfter.SendMessage (DTM_SETSYSTEMTIME, GDT_NONE , 0);
	m_cDatePickerBefore.SendMessage (DTM_SETSYSTEMTIME, GDT_NONE , 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIjaTable::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

void CuDlgIjaTable::PostNcDestroy() 
{
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
	delete this;
	CDialog::PostNcDestroy();
}


long CuDlgIjaTable::AddUser (LPCTSTR lpszUser)
{
	return (long)m_cComboUser.AddString(lpszUser);
}

void CuDlgIjaTable::CleanUser()
{
	m_cComboUser.ResetContent();
}

void CuDlgIjaTable::RefreshPaneTable(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner, LPCTSTR lpszTable, LPCTSTR lpszTableOwner)
{
	CWaitCursor doWaitCursor;
	BOOL bCleanSession = FALSE;
	CString strOldNode = m_queryTransactionInfo.GetNode();
	CString strOldDatabase;
	CString strOldDatabaseOwner;
	m_queryTransactionInfo.GetDatabase(strOldDatabase, strOldDatabaseOwner);
	if (!strOldNode.IsEmpty() && strOldNode.CompareNoCase(lpszNode)!=0)
		bCleanSession = TRUE;
	if (!strOldDatabase.IsEmpty() && strOldDatabase.CompareNoCase(lpszDatabase)!=0)
		bCleanSession = TRUE;
	if (bCleanSession)
		theApp.GetSessionManager().Cleanup();

	m_queryTransactionInfo.SetNode (lpszNode);
	m_queryTransactionInfo.SetDatabase (lpszDatabase, lpszDatabaseOwner);
	m_queryTransactionInfo.SetTable (lpszTable, lpszTableOwner);

	m_bViewButtonClicked = FALSE;
	m_cButtonSelectAll.ShowWindow (SW_HIDE);
	m_cButtonRedo.ShowWindow (SW_HIDE);
	m_cButtonRecover.ShowWindow (SW_HIDE);
	m_cListCtrl.ShowWindow (SW_HIDE);

	CleanListCtrl();
	SYSTEMTIME st;
	if (m_cDatePickerAfter.SendMessage (DTM_GETSYSTEMTIME, 0 , (LPARAM)&st) == GDT_VALID)
	{
		m_timeAfter = CTime(st);
		m_strAfter  = toAuditdbInputDate(m_timeAfter); //m_timeAfter.Format  (_T("%d-%m-%Y:%H:%M:%S"));
	}
	else
		m_strAfter = _T("");
	
	if (m_cDatePickerBefore.SendMessage (DTM_GETSYSTEMTIME, 0 , (LPARAM)&st) == GDT_VALID)
	{
		m_timeBefore = CTime(st);
		m_strBefore = toAuditdbInputDate(m_timeBefore); //m_timeBefore.Format (_T("%d-%m-%Y:%H:%M:%S"));
	}
	else
		m_strBefore = _T("");

	m_strAfter.MakeLower();
	m_strBefore.MakeLower();
	TRACE1 ("before: %s\n", m_strBefore);
	TRACE1 ("after : %s\n", m_strAfter);

#if defined (SIMULATION)
	return;
#endif
	//
	// Initialize the columns of the table as the list control header:
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
	if (!IJA_QueryColumn   (&m_queryTransactionInfo, m_listColumn))
	{
		m_bColumnHeaderOK = FALSE;
		return;
	}
	m_bColumnHeaderOK = TRUE;
	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof(lvcolumn));
	lvcolumn.mask = LVCF_TEXT;

	while (m_cListCtrl.GetColumn (nHeader, &lvcolumn))
	{
		//
		// Avoid looping for ever !
		if (!m_cListCtrl.DeleteColumn(nHeader))
			break;
	}
	
	BOOL bTextExtend = FALSE;
	CString strColum;
	SIZE  size;
	CDC*  pDC = m_cListCtrl.GetDC();
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	m_nColumnHeaderCount = m_listColumn.GetCount();
	int nCount = 0;
	CaColumn* pColumn = NULL;
	POSITION pos = m_listColumn.GetHeadPosition();
	while (pos != NULL)
	{
		pColumn = m_listColumn.GetNext (pos);
		strColum = pColumn->GetName();
		lvcolumn.fmt      = pColumn->IsColumnText()? LVCFMT_LEFT: LVCFMT_RIGHT;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strColum;
		lvcolumn.iSubItem = nHeader + nCount;
		bTextExtend = GetTextExtentPoint32 (pDC->m_hDC, strColum, strColum.GetLength(), &size);
		lvcolumn.cx = (bTextExtend && size.cx > 40)? size.cx+32: 40 +32;
		m_cListCtrl.InsertColumn (lvcolumn.iSubItem, &lvcolumn); 

		nCount++;
	}
#if defined (SORT_INDICATOR)
	CHeaderCtrl* pHdrCtrl= m_cListCtrl.GetHeaderCtrl();
	if (pHdrCtrl)
	{
		int i, nHeaderCount = pHdrCtrl->GetItemCount();
		pHdrCtrl->SetImageList(&m_ImageListOrder);
		HD_ITEM curItem;
		for (i=0; i<nHeaderCount; i++)
		{
			pHdrCtrl->GetItem(i, &curItem);
			curItem.mask= HDI_IMAGE | HDI_FORMAT;
			curItem.iImage= -1;
			curItem.fmt= HDF_LEFT | HDF_IMAGE | HDF_STRING /*|HDF_BITMAP_ON_RIGHT*/;
			pHdrCtrl->SetItem(i, &curItem);
		}
	}
#endif // SORT_INDICATOR
}

void CuDlgIjaTable::OnButtonView() 
{
	CWaitCursor doWaitCursor;
	try
	{
		if (!m_bViewButtonClicked)
		{
			m_bViewButtonClicked = TRUE;
			m_cButtonSelectAll.ShowWindow (SW_SHOW);
			m_cButtonRedo.ShowWindow (SW_SHOW);
			m_cButtonRecover.ShowWindow (SW_SHOW);
			m_cListCtrl.ShowWindow (SW_SHOW);
		}
		CleanListCtrl();

		UpdateData(TRUE);
		
		SYSTEMTIME st;
		if (m_cDatePickerAfter.SendMessage (DTM_GETSYSTEMTIME, 0 , (LPARAM)&st) == GDT_VALID)
		{
			m_timeAfter = CTime(st);
			m_strAfter  = toAuditdbInputDate(m_timeAfter); //m_timeAfter.Format  (_T("%d-%m-%Y:%H:%M:%S"));
		}
		else
			m_strAfter = _T("");
	
		if (m_cDatePickerBefore.SendMessage (DTM_GETSYSTEMTIME, 0 , (LPARAM)&st) == GDT_VALID)
		{
			m_timeBefore = CTime(st);
			m_strBefore = toAuditdbInputDate(m_timeBefore); //m_timeBefore.Format (_T("%d-%m-%Y:%H:%M:%S"));
		}
		else
			m_strBefore = _T("");
		m_strAfter.MakeLower();
		m_strBefore.MakeLower();
		TRACE1 ("before: %s\n", m_strBefore);
		TRACE1 ("after : %s\n", m_strAfter);

		m_queryTransactionInfo.SetBeforeAfter (m_strBefore, m_strAfter);
		m_queryTransactionInfo.SetUser(m_strUser);
		m_queryTransactionInfo.SetInconsistent(m_bInconsistent);
		m_queryTransactionInfo.SetWait(m_bWait);
		m_queryTransactionInfo.SetAfterCheckPoint (m_bAfterCheckPoint);
		m_queryTransactionInfo.SetCheckPointNumber (_ttoi (m_strCheckPointNo));
		m_queryTransactionInfo.SetAbortedTransaction (m_bAbortedTransaction);
		m_queryTransactionInfo.SetCommittedTransaction (m_bCommittedTransaction);
		m_strInitialUser = m_strUser;

		//
		// Opens the session
		CString strConnectedUser;
		CString strDatabase;
		CString strDatabaseOwner;
		m_queryTransactionInfo.GetDatabase (strDatabase, strDatabaseOwner);
		CaTemporarySession session (m_queryTransactionInfo.GetNode(), strDatabase);
		if (!session.IsConnected())
		{
			// 
			// Fail to get Session.
			CString strMsg;
			strMsg.LoadString(IDS_FAIL_TO_GETSESSION);
			AfxMessageBox (strMsg);
			return;
		}
		//
		// Get the connected user string. This function will throw exception if failed:
		IJA_QueryUserInCurSession (strConnectedUser);
		m_queryTransactionInfo.SetConnectedUser(strConnectedUser);
		session.Release();
	
		ASSERT (!m_listColumn.IsEmpty());
		if (m_listColumn.IsEmpty())
			return;

		BOOL bOk = IJA_QueryTableTransaction (&m_queryTransactionInfo, &m_listColumn, m_listMainTrans);
		if (bOk)
		{
			//
			// DIsplay Transactions:
			DisplayMainTransaction();
		}
		EnableButtons();
		EnableCheckPoint();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error, CuDlgIjaTable:OnButtonView, query data failed"));
	}
}



void CuDlgIjaTable::OnButtonRecover() 
{
	BOOL bError = FALSE;
	CString strMsg;
	CWaitCursor doWaitCursor;
	if (m_bInconsistent)
	{
		strMsg.LoadString (IDS_RECOVERREDO_IMPOSSIBLE);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
		return;
	}

	CxDlgRecover dlg;
	CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr = dlg.GetListTransaction();
	//
	// Prepare the selected transactions:
	GetSelectedTransactions (ltr); // Transactions of the current table.
	if (!RCRD_CheckValidateTransaction (ltr))
		return;

	//
	// Query other related transactions of other tables:
	CTypedPtrList < CObList, CaBaseTransactionItemData* > ltr2;
	CTypedPtrList < CObList, CaDatabaseTransactionItemData* > listRelatedTransaction;
	CString strTable;
	CString strTableOwner;
	m_queryTransactionInfo.GetTable (strTable, strTableOwner);

	try
	{
		if (!IJA_QueryDatabaseTransaction (&m_queryTransactionInfo, listRelatedTransaction))
		{
			//
			// Failed to query the related transactions.
			strMsg.LoadString (IDS_FAIL_TO_QUERY_RELATED_TRANS);

			AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		BOOL bInitLsnEnd = FALSE;
		POSITION pos = listRelatedTransaction.GetHeadPosition();
		while (pos != NULL)
		{
			CaDatabaseTransactionItemData* pDbTr = listRelatedTransaction.GetNext (pos);
			CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& ldt = pDbTr->GetDetailTransaction();
			POSITION p = ldt.GetHeadPosition();
			while (p != NULL)
			{
				CaDatabaseTransactionItemDataDetail* pDetail = ldt.GetNext (p);
				//
				// If for each detail transaction, if this transaction is the same as the one of the current
				// table (eg in the list 'ltr') then take this detail transaction too:
				CString strItem = pDetail->GetTable();
				CString strItemOwner = pDetail->GetTableOwner();
				if (strTable.CompareNoCase (strItem) == 0 && strTableOwner.CompareNoCase (strItemOwner) == 0)
				{
					CString strLsnEnd = pDbTr->GetLsnEnd();
					POSITION px = ltr.GetHeadPosition();
					while (px != NULL && !bInitLsnEnd)
					{
						CaBaseTransactionItemData* pItem = ltr.GetNext(px);
						pItem->SetLsnEnd(strLsnEnd);
					}
					bInitLsnEnd = TRUE;
					continue;
				}
				if (ExistTransaction (pDetail, ltr))
				{
					pDetail->SetLsnEnd(pDbTr->GetLsnEnd());
					ltr2.AddTail ((CaBaseTransactionItemData*)pDetail);
				}
			}
		}

		while (!ltr2.IsEmpty())
		{
			CaBaseTransactionItemData* pDetail = ltr2.RemoveHead();
			ltr.AddTail (pDetail);
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
		bError = TRUE;
	}
	catch (...)
	{
		AfxMessageBox(_T("System error: CuDlgIjaTable::OnButtonRedo(), ReDo failed"));
		bError = TRUE;
	}

	UINT* pArraySelectedItem = NULL;
	int   nSelectedCount = 0;

	if (!bError)
	{
		GetSelectedItems (pArraySelectedItem, nSelectedCount);
		dlg.SetQueryTransactionInfo(&m_queryTransactionInfo);
		dlg.SetInitialSelection (pArraySelectedItem, nSelectedCount);
		//
		// Luanch the dialog box:
		dlg.DoModal();
	}

	if (pArraySelectedItem)
		delete [] pArraySelectedItem;
	while (!listRelatedTransaction.IsEmpty())
		delete listRelatedTransaction.RemoveHead();
}

void CuDlgIjaTable::OnButtonRedo() 
{
	BOOL bError = FALSE;
	CString strMsg;
	CWaitCursor doWaitCursor;
	if (m_bInconsistent)
	{
		strMsg.LoadString (IDS_RECOVERREDO_IMPOSSIBLE);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
		return;
	}

	CxDlgRedo dlg;
	CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr = dlg.GetListTransaction();
	//
	// Prepare the selected transactions:
	GetSelectedTransactions (ltr);
	if (!RCRD_CheckValidateTransaction (ltr))
		return;

	//
	// Query other related transactions of other tables:
	CTypedPtrList < CObList, CaBaseTransactionItemData* > ltr2;
	CTypedPtrList < CObList, CaDatabaseTransactionItemData* > listRelatedTransaction;
	CString strTable;
	CString strTableOwner;
	m_queryTransactionInfo.GetTable (strTable, strTableOwner);

	try
	{
		if (!IJA_QueryDatabaseTransaction (&m_queryTransactionInfo, listRelatedTransaction))
		{
			//
			// Failed to query the related transactions.
			strMsg.LoadString (IDS_FAIL_TO_QUERY_RELATED_TRANS);

			AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
			return;
		}

		BOOL bInitLsnEnd = FALSE;
		POSITION pos = listRelatedTransaction.GetHeadPosition();
		while (pos != NULL)
		{
			CaDatabaseTransactionItemData* pDbTr = listRelatedTransaction.GetNext (pos);
			CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& ldt = pDbTr->GetDetailTransaction();
			POSITION p = ldt.GetHeadPosition();
			while (p != NULL)
			{
				CaDatabaseTransactionItemDataDetail* pDetail = ldt.GetNext (p);
				//
				// If for each detail transaction, if this transaction is the same as the one of the current
				// table (eg in the list 'ltr') then take this detail transaction too:
				CString strItem = pDetail->GetTable();
				CString strItemOwner = pDetail->GetTableOwner();
				if (strTable.CompareNoCase (strItem) == 0 && strTableOwner.CompareNoCase (strItemOwner) == 0)
				{
					CString strLsnEnd = pDbTr->GetLsnEnd();
					POSITION px = ltr.GetHeadPosition();
					while (px != NULL && !bInitLsnEnd)
					{
						CaBaseTransactionItemData* pItem = ltr.GetNext(px);
						pItem->SetLsnEnd(strLsnEnd);
					}
					bInitLsnEnd = TRUE;
					continue;
				}
				if (ExistTransaction (pDetail, ltr))
				{
					ltr2.AddTail ((CaBaseTransactionItemData*)pDetail);
				}
			}
		}
		while (!ltr2.IsEmpty())
		{
			CaBaseTransactionItemData* pDetail = ltr2.RemoveHead();
			ltr.AddTail (pDetail);
		}
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
		bError = TRUE;
	}
	catch (...)
	{
		bError = TRUE;
		AfxMessageBox (_T("System error: CuDlgIjaTable::OnButtonRedo(), ReDo failed"));
	}

	UINT* pArraySelectedItem = NULL;
	int   nSelectedCount = 0;
	if (!bError)
	{
		GetSelectedItems (pArraySelectedItem, nSelectedCount);
		dlg.SetQueryTransactionInfo(&m_queryTransactionInfo);
		dlg.SetInitialSelection (pArraySelectedItem, nSelectedCount);
		//
		// Luanch the dialog box:
		dlg.DoModal();
	}

	if (pArraySelectedItem)
		delete [] pArraySelectedItem;
	while (!listRelatedTransaction.IsEmpty())
		delete listRelatedTransaction.RemoveHead();
}

void CuDlgIjaTable::CleanListCtrl()
{
	m_cListCtrl.DeleteAllItems();
	while (!m_listMainTrans.IsEmpty())
		delete m_listMainTrans.RemoveHead();
}


void CuDlgIjaTable::ResizeControls()
{
	if (!IsWindow(m_cListCtrl.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);

	r = m_rcInitialButtonRedo;
	r.right  = rDlg.right  - m_sizeMargin.cx;
	r.left   = r.right - m_rcInitialButtonRedo.Width();
	r.bottom = rDlg.bottom - 4;
	r.top    = r.bottom - m_rcInitialButtonRecover.Height();
	m_cButtonRedo.MoveWindow (r);

	int nX1 = r.left;
	r = m_rcInitialButtonRecover;
	r.right = nX1 - 1;
	r.left  = r.right - m_rcInitialButtonRecover.Width();
	r.bottom = rDlg.bottom - 4;
	r.top    = r.bottom - m_rcInitialButtonRecover.Height();
	m_cButtonRecover.MoveWindow (r);

	int nY1 = r.top;
	r = m_rcInitialListCtrl;
	r.right  = rDlg.right   - m_sizeMargin.cx;
	r.bottom = nY1  - 4;
	m_cListCtrl.MoveWindow (r);
}

void CuDlgIjaTable::OnDestroy() 
{
	CleanListCtrl();
	CDialog::OnDestroy();
}

void CuDlgIjaTable::OnButtonSelectAll() 
{
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		m_cListCtrl.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CuDlgIjaTable::GetSelectedTransactions (CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr)
{
	int i, nCount;
	int nFound = -1;
	CaTableTransactionItemData* pItemTable= NULL;
	CString strTable;
	CString strTableOwner;
	CString strData;

	m_queryTransactionInfo.GetTable (strTable, strTableOwner);

	nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		if (m_cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
		{
			pItemTable = (CaTableTransactionItemData*)m_cListCtrl.GetItemData (i);
			if (!pItemTable)
				continue;
			unsigned long ltr1L, ltr1H;
			unsigned long ltr2L, ltr2H;

			for (int j=0; j<nCount; j++)
			{
				CaTableTransactionItemData* pItem = (CaTableTransactionItemData*)m_cListCtrl.GetItemData(j);
				if (!pItem)
					continue;

				pItem->GetTransactionID(ltr1H, ltr1L);
				pItemTable->GetTransactionID(ltr2H, ltr2L);

				if (ltr1H == ltr2H && ltr1L == ltr2L)
				{
					pItem->SetTable (strTable);
					pItem->SetTableOwner (strTableOwner);
					if (ltr.Find(pItem) == NULL)
						ltr.AddTail (pItem);
				}
			}
		}
	}
}

void CuDlgIjaTable::GetSelectedItems (UINT*& pArraySelectedItem, int& nSelectedCount)
{
	try
	{
		int i, nCount = m_cListCtrl.GetItemCount();
		int idx = 0, nSelCount = 0;

		nSelCount = m_cListCtrl.GetSelectedCount();
		if (nSelCount == 0)
			return;
		nSelectedCount = nSelCount;
		pArraySelectedItem = new UINT [nSelCount];
		memset (pArraySelectedItem, 0, (UINT)nSelCount);
		for (i=0; i<nCount; i++)
		{
			if (m_cListCtrl.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
			{
				pArraySelectedItem[idx] = (UINT)m_cListCtrl.GetItemData(i);
				idx++;
			}
		}
	}
	catch (...)
	{
		TRACE0 ("CuDlgIjaTable::GetSelectedItems \n");
	}
}

void CuDlgIjaTable::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	if (pNMListView->iSubItem == -1)
		return;

	BOOL bDateCol = FALSE;
	if (pNMListView->iSubItem == 2)
		bDateCol = TRUE;
	else
	if (pNMListView->iSubItem >= nHeader)
	{
		CaColumn* pCol = GetNthColumn (&m_listColumn, pNMListView->iSubItem - (nHeader - 1));
		if (pCol && INGRESII_GetSqlDisplayType (pCol->GetType()) == SQLACT_DATE)
			bDateCol = TRUE;
	}

	if (bDateCol && theApp.m_dateFormat == II_DATE_MULTINATIONAL && theApp.m_dateCenturyBoundary == 0)
	{
		//
		// The environment variable II_DATE_CENTURY_BOUNDARY is not set.\n
		// And the current date format is II_DATE_MULTINATIONAL, so the sort on this column will not work correctly.
		CString strMsg;
		strMsg.LoadString(IDS_NOTSET_II_DATE_CENTURY_BOUNDARY);

		AfxMessageBox (strMsg);
	}

	CWaitCursor doWaitCursor;
	if (m_sortparam.m_nItem == pNMListView->iSubItem)
	{
		m_sortparam.m_bAsc = !m_sortparam.m_bAsc;
	}
	else
		m_sortparam.m_bAsc = TRUE;
	m_sortparam.m_nItem = pNMListView->iSubItem;
	
	SORTTABLECOLUMN SortCol;
	memcpy (&(SortCol.m_sortParam), &m_sortparam, sizeof(m_sortparam));
	SortCol.m_pListColumn = &m_listColumn;

	m_cListCtrl.SortItems(CompareSubItem, (LPARAM)&SortCol);
	ChangeSortIndicator(&m_cListCtrl, &m_sortparam);
}


static CString GetNthItem (CStringList& listStr, int nItem)
{
	CString strItem;
	int nStart = 0;
	POSITION pos = listStr.GetHeadPosition();
	while (pos != NULL)
	{
		strItem = listStr.GetNext (pos);
		nStart++;
		if (nStart == nItem)
			break;
	}
	return strItem;
}

CaColumn* GetNthColumn (CTypedPtrList<CObList, CaColumn*>* pListColumn, int nItem)
{
	int nStart = 0;
	ASSERT (pListColumn);
	if (!pListColumn)
		return 0;

	POSITION pos = pListColumn->GetHeadPosition();
	while (pos != NULL)
	{
		CaColumn* pCol = pListColumn->GetNext (pos);
		nStart++;
		if (nStart == nItem)
			return pCol;
	}

	return NULL;
}

int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC;
	CaListCtrlColorItemData* lpItem1 = (CaListCtrlColorItemData*)lParam1;
	CaListCtrlColorItemData* lpItem2 = (CaListCtrlColorItemData*)lParam2;
	if (lpItem1 == NULL || lpItem2 == NULL)
		return 0;
	CaTableTransactionItemData* pItem1 = (CaTableTransactionItemData*)lpItem1->m_lUserData;
	CaTableTransactionItemData* pItem2 = (CaTableTransactionItemData*)lpItem2->m_lUserData;

	SORTTABLECOLUMN* pSortCol = (SORTTABLECOLUMN*)lParamSort;
	ASSERT (pSortCol);
	if (!pSortCol)
		return 0;

	SORTPARAMS*   pSr = (SORTPARAMS*)(&pSortCol->m_sortParam);
	BOOL bAsc = pSr? pSr->m_bAsc: TRUE;

	if (pItem1 && pItem2)
	{
		nC = bAsc? 1 : -1;
		if ((pSr->m_nItem > -1) && (pSr->m_nItem < nHeader))
			return nC * pItem1->Compare (pItem2, pSr->m_nItem);
		else
		{
			//
			// Compare on the real column data type:
			CaColumn* pCol = GetNthColumn(pSortCol->m_pListColumn, pSr->m_nItem - (nHeader - 1));
			ASSERT (pCol);
			if (!pCol)
				return 0;

			CStringList& listStr1 = pItem1->GetListData();
			CStringList& listStr2 = pItem2->GetListData();
			CString str1 = GetNthItem(listStr1,  pSr->m_nItem - (nHeader - 1));
			CString str2 = GetNthItem(listStr2, pSr->m_nItem - (nHeader - 1));

			int nDisplayType = INGRESII_GetSqlDisplayType (pCol->GetType());
			switch (nDisplayType)
			{
			case SQLACT_NUMBER:
				{
					long lValue1 = _ttol (str1);
					long lValue2 = _ttol (str2);
					int nCP = (lValue1 > lValue2) ? 1 : (lValue1 < lValue2) ? -1: 0;
					return nC * nCP;
				}
				break;
			case SQLACT_NUMBER_FLOAT:
				{
					double dValue1 = atof (str1);
					double dValue2 = atof (str2);
					int nCP = (dValue1 > dValue2) ? 1 : (dValue1 < dValue2) ? -1: 0;
					return nC * nCP;
				}
				break;
			case SQLACT_TEXT:
				return nC * str1.CollateNoCase (str2);
				break;
			case SQLACT_DATE:
				return nC * IJA_CompareDate (str1, str2, TRUE);
				break;
			}
		}
	}
	else
	{
		nC = pItem1? 1: pItem2? -1: 0;
		return bAsc? nC: (-1)*nC;
	}

	return 0;
}

void CuDlgIjaTable::EnableButtons()
{
	int  nSelCount = m_cListCtrl.GetSelectedCount();
	BOOL bEnable = (nSelCount > 0 && m_bEnableRecoverRedo)? TRUE: FALSE;
	m_cButtonRedo.EnableWindow (bEnable);
	m_cButtonRecover.EnableWindow (bEnable);
}

void CuDlgIjaTable::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	EnableButtons();
}

void CuDlgIjaTable::ChangeSortIndicator(CListCtrl* pListCtrl, SORTPARAMS* pSp)
{
#if defined (SORT_INDICATOR)
	HD_ITEM curItem;
	CHeaderCtrl* pHdrCtrl= pListCtrl->GetHeaderCtrl();
	if (!pHdrCtrl)
		return;

	curItem.mask= HDI_IMAGE | HDI_FORMAT;
	curItem.iImage= -1;
	curItem.fmt= HDF_LEFT | HDF_IMAGE | HDF_STRING /*| HDF_BITMAP_ON_RIGHT*/;
	int nCount = pHdrCtrl->GetItemCount();
	for (int i = 0; i<nCount; i++)
	{
		pHdrCtrl->GetItem(i, &curItem);
		curItem.mask= HDI_IMAGE | HDI_FORMAT;
		curItem.iImage= -1;
		pHdrCtrl->SetItem(i, &curItem);
	}
	if (pSp->m_nItem != -1)
	{
		pHdrCtrl->GetItem(pSp->m_nItem, &curItem);
		curItem.iImage = pSp->m_bAsc? IM_HEADER_DOWN: IM_HEADER_UP;
		pHdrCtrl->SetItem(pSp->m_nItem, &curItem);
	}
#endif // SORT_INDICATOR
}

void CuDlgIjaTable::OnButtonMoveLog2Journals() 
{
	TRACE0 ("CuDlgIjaTable::OnButtonMoveLog2Journals() \n");
	CWaitCursor doWaitCursor;
	CString strImaDbConnection;
	CString strMsg;
	
	try
	{
		strImaDbConnection.Format (_T("%s::imadb"), (LPCTSTR)m_queryTransactionInfo.GetNode ());
		CaTemporarySession session (m_queryTransactionInfo.GetNode (), _T("imadb"));
		if (!session.IsConnected())
		{
			//
			// Cannot connect to <%s>
			CString strFormat;
			strFormat.LoadString(IDS_CANNOT_CONNECT_TO);
			strMsg.Format (strFormat, (LPCTSTR)strImaDbConnection);
			AfxMessageBox (strMsg);
			return;
		}

		IJA_ActivateLog2Journal ();
		Sleep(1000);

		while(1)
		{
			CString csCurStatus;
			IJA_LogFileStatus(csCurStatus);
			if (csCurStatus.Find(_T("ARCHIVE")) == -1  )
				break;
			Sleep(1000);
		}
		session.Release(SESSION_COMMIT);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error: CuDlgIjaTable::OnButtonMoveLog2Journals failed"), MB_OK|MB_ICONEXCLAMATION);
	}
}

void CuDlgIjaTable::EnableCheckPoint()
{
	BOOL bCheck = (m_cCheckAfterCheckPoint.GetCheck() == 1)? TRUE: FALSE;
	m_cEditAfterCheckPoint.EnableWindow (bCheck);
	m_ButtonCheckpointList.EnableWindow (bCheck);
}

void CuDlgIjaTable::OnCheckAfterCheckPoint() 
{
	EnableCheckPoint();
	OnInputParamChange(IDCT_CHECK_CHECKPOINT);
}

void CuDlgIjaTable::OnCheckAbortedTransaction() 
{
	OnInputParamChange(IDCT_ABORT_TRANS);
}

void CuDlgIjaTable::OnCheckCommittedTransaction() 
{
	OnInputParamChange(IDCT_COMMIT_TRANS);
}

void CuDlgIjaTable::OnSelchangeComboUser() 
{
	CString strOld = m_strUser;
	UpdateData(TRUE);
	if (strOld.CompareNoCase(m_strUser) != 0)
	{
		if (m_strInitialUser.CompareNoCase (theApp.m_strAllUser) != 0)
			OnInputParamChange(0); // Requery
		else
			OnInputParamChange(IDCT_USER); // Update to the specific user (all users have been queried)
	}
}


void CuDlgIjaTable::OnDatetimechangeBefore(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CString strTm;
	SYSTEMTIME st;
	strTm = m_strBefore;
	*pResult = 0;

	UpdateData(TRUE);
	if (m_cDatePickerBefore.SendMessage (DTM_GETSYSTEMTIME, 0 , (LPARAM)&st) == GDT_VALID)
	{
		m_timeBefore = CTime(st);
		m_strBefore  = toAuditdbInputDate(m_timeBefore); //m_timeBefore.Format  (_T("%d-%m-%Y:%H:%M:%S"));
	}
	else
		m_strBefore = _T("");

	if (!m_strBefore.IsEmpty() && m_strBefore.CompareNoCase (strTm) != 0)
	{
		TRACE1 ("CuDlgIjaTable::OnDatetimechangeBefore...New time = %s \n", strTm);
		OnInputParamChange(IDCT_BEFOR);
	}
}


void CuDlgIjaTable::OnDatetimechangeAfter(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CString strTm;
	SYSTEMTIME st;
	strTm = m_strAfter;
	*pResult = 0;

	UpdateData(TRUE);
	if (m_cDatePickerAfter.SendMessage (DTM_GETSYSTEMTIME, 0 , (LPARAM)&st) == GDT_VALID)
	{
		m_timeAfter = CTime(st);
		m_strAfter  = toAuditdbInputDate(m_timeAfter); //m_timeAfter.Format  (_T("%d-%m-%Y:%H:%M:%S"));
	}
	else
		m_strAfter = _T("");

	if (!m_strAfter.IsEmpty() && m_strAfter.CompareNoCase (strTm) != 0)
	{
		TRACE1 ("CuDlgIjaTable::OnDatetimechangeAfter...New time = %s \n", strTm);
		OnInputParamChange(IDCT_AFTER);
	}
}

void CuDlgIjaTable::OnChangeEditAfterCheckPoint() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	OnInputParamChange(IDCT_EDIT_CHECKPOINT);
}

void CuDlgIjaTable::OnCheckWait() 
{
	OnInputParamChange(IDCT_WAIT);
}


void CuDlgIjaTable::OnInputParamChange(int idct)
{
	CString strNode = m_queryTransactionInfo.GetNode();
	CString strDatabase;
	CString strDatabaseOwner;
	CString strTable;
	CString strTableOwner;
	m_queryTransactionInfo.GetDatabase (strDatabase, strDatabaseOwner);
	m_queryTransactionInfo.GetTable (strTable, strTableOwner);

	switch (idct)
	{
	case IDCT_AFTER:
	case IDCT_BEFOR:
	case IDCT_CHECK_CHECKPOINT:
	case IDCT_EDIT_CHECKPOINT:
	case IDCT_WAIT:
		if (strNode.IsEmpty() || strDatabase.IsEmpty() ||  strTable.IsEmpty())
			break;
		RefreshPaneTable(strNode, strDatabase, strDatabaseOwner, strTable, strTableOwner);
		break;

	case IDCT_ABORT_TRANS:
	case IDCT_COMMIT_TRANS:
	case IDCT_USER:
		UpdateData (TRUE);
		m_cListCtrl.DeleteAllItems();
		DisplayMainTransaction();
		EnableButtons();
		EnableCheckPoint();
		break;
	default:
		if (strNode.IsEmpty() || strDatabase.IsEmpty() ||  strTable.IsEmpty())
			break;
		RefreshPaneTable(strNode, strDatabase, strDatabaseOwner, strTable, strTableOwner);
		break;
	}

	TRACE0("CuDlgIjaTable::OnInputParamChange() \n");
}

void CuDlgIjaTable::DisplayMainTransaction()
{
	//
	// DIsplay Transactions:
	int idx, nCount = 0, nImage = 0;
	POSITION pos = m_listMainTrans.GetHeadPosition();
	while (pos !=  NULL)
	{
		CaTableTransactionItemData* pItem = m_listMainTrans.GetNext (pos);
		//
		// Check to see if this transaction matchs the filter
		// if not just skip it.
		if (!m_bAbortedTransaction && !pItem->GetCommit())
			continue; // Skip the rollback transaction

		if (!m_bCommittedTransaction && pItem->GetCommit())
			continue; // Skip the commit transaction
		
		if (m_strUser.CompareNoCase(theApp.m_strAllUser) != 0 && m_strUser.CompareNoCase (pItem->GetUser()) != 0)
			continue;

		nCount = m_cListCtrl.GetItemCount();
		nImage = pItem->GetImageId();

		idx = m_cListCtrl.InsertItem (nCount, _T(""), nImage);
		if (idx != -1)
		{
			switch (pItem->GetAction())
			{
			case T_DELETE:
				IJA_ColorItem(&m_cListCtrl, idx, theApp.m_property.TRGB_DELETE());
				break;
			case T_INSERT:
				IJA_ColorItem(&m_cListCtrl, idx, theApp.m_property.TRGB_INSERT());
				break;
			case T_BEFOREUPDATE:
				IJA_ColorItem(&m_cListCtrl, idx, theApp.m_property.TRGB_BEFOREUPDATE());
				break;
			case T_AFTERUPDATE:
				IJA_ColorItem(&m_cListCtrl, idx, theApp.m_property.TRGB_AFTEREUPDATE());
				break;
			case T_CREATETABLE:
			case T_ALTERTABLE:
			case T_DROPTABLE:
			case T_RELOCATE:
			case T_MODIFY:
			case T_INDEX:
				IJA_ColorItem(&m_cListCtrl, idx, theApp.m_property.TRGB_OTHERS());
				break;
			default:
				break;
			}
			m_cListCtrl.SetItemData (idx, (DWORD)pItem);

			m_cListCtrl.SetItemText (idx, 0, pItem->GetStrTransactionID());
			m_cListCtrl.SetItemText (idx, 1, pItem->GetLsn());
			m_cListCtrl.SetItemText (idx, 2, pItem->GetDateLocale());
			m_cListCtrl.SetItemText (idx, 3, pItem->GetUser());
			m_cListCtrl.SetItemText (idx, 4, pItem->GetStrOperation());
#if defined (SIMULATION)
			m_cListCtrl.SetItemText (idx, 5, pItem->GetName());
#else
			//
			// Display the real data:
			CStringList& listdata = pItem->GetListData();
			POSITION p = listdata.GetHeadPosition();
			int nCounter = 0;
			while (p != NULL && nCounter < m_nColumnHeaderCount)
			{
				CString strItem = listdata.GetNext (p);
				m_cListCtrl.SetItemText (idx, nHeader + nCounter, strItem);

				nCounter++;
			}
#endif
		}
	}
}


void CuDlgIjaTable::OnKillfocusEditAfterCheckPoint() 
{
	CString m_strText;
	m_cEditAfterCheckPoint.GetWindowText(m_strText);
	m_strText.TrimLeft();
	m_strText.TrimRight();

	int nVal = _ttoi(m_strText);
	if (nVal <= 0)
		m_cEditAfterCheckPoint.SetWindowText(_T("1"));
}

void CuDlgIjaTable::OnButtonCheckpointList() 
{
	CString csDbName,csDBOwner,csConnectedUser;
	UpdateData(TRUE);

	m_queryTransactionInfo.GetDatabase(csDbName, csDBOwner);
	csConnectedUser = m_queryTransactionInfo.GetConnectedUser();
	//
	// Opens the session
	CaTemporarySession session (m_queryTransactionInfo.GetNode(), csDbName);
	//
	// Get the connected user string. This function will throw exception if failed:
	IJA_QueryUserInCurSession (csConnectedUser);
	m_queryTransactionInfo.SetConnectedUser(csConnectedUser);
	session.Release();

	CxDlgCheckPointLst Dlg;
	Dlg.m_csCurDBName    = csDbName;
	Dlg.m_csCurDBOwner   = csConnectedUser;
	Dlg.m_csCurVnodeName = m_queryTransactionInfo.GetNode();

	Dlg.SetSelectedCheckPoint(m_strCheckPointNo);
	Dlg.DoModal();
	m_strCheckPointNo = Dlg.GetSelectedCheckPoint();
	UpdateData(FALSE);
}
