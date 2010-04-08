/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijadbase.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modeless Dialog represents the Database Pane 
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 12-Jun-2001 (uk$so01)
**    Sir # 101169 (additional changes). Display the title of
**    the Detail transaction: "Detail for Transaction XXX".
** 31-Oct-2001 (hanje04)
**    Move declaration of i outside for loops to stop non-ANSI compiler 
**    errors on Linux.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 11-Jul-2002 (hanje04)
**    In 'OnButtonMoveLog2Journals' cannot pass CStrings through %s, cast
**    with (LPCTSTR)
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 02-Dec-2003 (schph01)
**    SIR #111389 Implement new button for displayed the checkpoint list.
** 25-Juin-2004 (uk$so01)
**    BUG #112556 / ISSUE 13524649
**    Add the support of displaying the detail of multiple selected 
**    transactions in the upper list of main transactions.
** 13-Oct-2004 (uk$so01)
**    BUG #113226 / ISSUE 13720088: The DBMS connections should be disconnected
**    if they ere not used.
** 01-Dec-2004 (uk$so01)
**    IJA BUG #113551
**    Ija should internally specify the fixed date format expected by auditdb 
**    command independently of the LOCALE setting.
**/

#include "stdafx.h"
#include "ijactrl.h"
#include "ijadbase.h"
#include "ijactdml.h"
#include "drecover.h"
#include "dredo.h"
#include "ijadmlll.h"
#include "rcrdtool.h"
#include "ckplst.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

static int CALLBACK CompareSubItem1 (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
static int CALLBACK CompareSubItem2 (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

static BOOL ExistTransactionID(CStringList& listTransID, CString& strTransID)
{
	CString strItem;
	POSITION pos = listTransID.GetHeadPosition();
	while (pos != NULL)
	{
		strItem = listTransID.GetNext (pos);
		if (strItem.CompareNoCase (strTransID) == 0)
			return TRUE;
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CuDlgIjaDatabase dialog


CuDlgIjaDatabase::CuDlgIjaDatabase(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIjaDatabase::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIjaDatabase)
	m_bCheckGroupByTransaction = TRUE;
	m_strTxTitle = _T("");
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
	m_bSelectAll = FALSE;
	m_sortparam1.m_bAsc  = TRUE;
	m_sortparam1.m_nItem = -1;
	m_sortparam2.m_bAsc  = TRUE;
	m_sortparam2.m_nItem = -1;
	m_bViewButtonClicked = FALSE;
	m_strAfter = _T("");
	m_strBefore = _T("");
	m_bEnableRecoverRedo = TRUE;
	m_strInitialUser = theApp.m_strAllUser;
}


void CuDlgIjaDatabase::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIjaDatabase)
	DDX_Control(pDX, IDC_CHECKPOINT_LIST_BUTTON, m_checkpoint_list_button);
	DDX_Control(pDX, IDC_CHECK2, m_cCheckAfterCheckPoint);
	DDX_Control(pDX, IDC_EDIT3, m_cEditAfterCheckPoint);
	DDX_Control(pDX, IDC_DATETIMEPICKER2, m_cDatePickerBefore);
	DDX_Control(pDX, IDC_DATETIMEPICKER1, m_cDatePickerAfter);
	DDX_Control(pDX, IDC_BUTTON4, m_cButtonSelectAll);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonRedo);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonRecover);
	DDX_Control(pDX, IDC_STATIC_DETAIL4TX, m_cStaticTxTitle);
	DDX_Control(pDX, IDC_CHECK4, m_cCheckGroupByTransaction);
	DDX_Control(pDX, IDC_COMBO1, m_cComboUser);
	DDX_Check(pDX, IDC_CHECK4, m_bCheckGroupByTransaction);
	DDX_Text(pDX, IDC_STATIC_DETAIL4TX, m_strTxTitle);
	DDX_Check(pDX, IDC_CHECK2, m_bAfterCheckPoint);
	DDX_Check(pDX, IDC_CHECK3, m_bWait);
	DDX_CBString(pDX, IDC_COMBO1, m_strUser);
	DDX_Text(pDX, IDC_EDIT3, m_strCheckPointNo);
	DDX_Check(pDX, IDC_CHECK5, m_bAbortedTransaction);
	DDX_Check(pDX, IDC_CHECK6, m_bCommittedTransaction);
	//}}AFX_DATA_MAP
//	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER2, m_timeBefore);
//	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER1, m_timeAfter);
}


BEGIN_MESSAGE_MAP(CuDlgIjaDatabase, CDialog)
	//{{AFX_MSG_MAP(CuDlgIjaDatabase)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonView)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonRecover)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonRedo)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_BN_CLICKED(IDC_CHECK4, OnCheckGroupByTransaction)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON4, OnButtonSelectAll)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList1)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST2, OnColumnclickList2)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST2, OnItemchangedList2)
	ON_BN_CLICKED(IDC_BUTTON8, OnButtonMoveLog2Journals)
	ON_NOTIFY(LVN_ITEMCHANGING, IDC_LIST2, OnItemchangingList2)
	ON_BN_CLICKED(IDC_CHECK2, OnCheckAfterCheckPoint)
	ON_BN_CLICKED(IDC_CHECK5, OnCheckAbortedTransaction)
	ON_BN_CLICKED(IDC_CHECK6, OnCheckCommittedTransaction)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboUser)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER2, OnDatetimechangeBefore)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER1, OnDatetimechangeAfter)
	ON_EN_CHANGE(IDC_EDIT3, OnChangeEditAfterCheckPoint)
	ON_BN_CLICKED(IDC_CHECK3, OnCheckWait)
	ON_EN_KILLFOCUS(IDC_EDIT3, OnKillfocusEditAfterCheckPoint)
	ON_BN_CLICKED(IDC_CHECKPOINT_LIST_BUTTON, OnCheckpointListButton)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_CTRLxSHIFT_UP, OnListCtrl1KeyUp)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIjaDatabase message handlers

void CuDlgIjaDatabase::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}


long CuDlgIjaDatabase::AddUser (LPCTSTR lpszUser)
{
	return (long)m_cComboUser.AddString(lpszUser);
}

void CuDlgIjaDatabase::CleanUser()
{
	m_cComboUser.ResetContent();
}

void CuDlgIjaDatabase::RefreshPaneDatabase(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszDatabaseOwner)
{
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
	
	m_bViewButtonClicked = FALSE;
	m_cButtonSelectAll.ShowWindow (SW_HIDE);
	m_cButtonRedo.ShowWindow (SW_HIDE);
	m_cButtonRecover.ShowWindow (SW_HIDE);
	m_cListCtrl1.ShowWindow (SW_HIDE);
	m_cListCtrl2.ShowWindow (SW_HIDE);
	m_cCheckGroupByTransaction.ShowWindow (SW_HIDE);
	m_cStaticTxTitle.ShowWindow (SW_HIDE);

	CleanListCtrl1();
	CleanListCtrl2();

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
	TRACE1 ("before: %s\n", m_strBefore);
	TRACE1 ("after : %s\n", m_strAfter);
}


BOOL CuDlgIjaDatabase::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ImageList.Create (IDB_IJAIMAGELIST,  16, 0, RGB(255,0,255));
	m_ImageList2.Create (IDB_IJAIMAGELIST2, 16, 0, RGB(255,0,255));
	m_ImageListOrder.Create (IDB_HEADERORDER, 16, 0, RGB(255,0,255));

	VERIFY (m_cListCtrl1.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_cListCtrl1.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl1.m_hWnd, GWL_STYLE, style);
	m_cListCtrl1.SetFullRowSelected (TRUE);
	m_cListCtrl1.SetLineSeperator(FALSE);
	m_cListCtrl1.SetImageList  (&m_ImageList, LVSIL_SMALL);

	VERIFY (m_cListCtrl2.SubclassDlgItem (IDC_LIST2, this));
	style = GetWindowLong (m_cListCtrl2.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListCtrl2.m_hWnd, GWL_STYLE, style);
	m_cListCtrl2.SetFullRowSelected (TRUE);
	m_cListCtrl2.SetLineSeperator(FALSE);
	m_cListCtrl2.SetImageList  (&m_ImageList2, LVSIL_SMALL);
	m_cListCtrl1.ActivateColorItem();
	m_cListCtrl2.ActivateColorItem();

	const int nHeader = 9;
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[nHeader] =
	{
		{IDS_HEADER_TRANSACTION,145, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_USER,        70, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_BEGIN,       80, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_END,         80, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_DURATION,    80, LVCFMT_RIGHT,FALSE},
		{IDS_HEADER_INSERT,      60, LVCFMT_RIGHT,FALSE},
		{IDS_HEADER_UPDATE,      60, LVCFMT_RIGHT,FALSE},
		{IDS_HEADER_DELETE,      60, LVCFMT_RIGHT,FALSE},
		{IDS_HEADER_TOTAL,       60, LVCFMT_RIGHT,FALSE},
	};

	CString strHeader;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	int i=0;
	for (i=0; i<nHeader; i++)
	{
		strHeader.LoadString (lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl1.InsertColumn (i, &lvcolumn); 
	}
	

	const int nHeader2 = 6;
	LSCTRLHEADERPARAMS2 lsp2[nHeader] =
	{
		{IDS_HEADER_TRANSACTION, 145, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_LSN,         145, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_USER,         70, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_ACTION,      100, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_TABLE,       120, LVCFMT_LEFT, FALSE},
		{IDS_HEADER_DATA,        200, LVCFMT_LEFT, FALSE},
	};

	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<nHeader2; i++)
	{
		strHeader.LoadString (lsp2[i].m_nIDS);
		lvcolumn.fmt      = lsp2[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp2[i].m_cxWidth;
		m_cListCtrl2.InsertColumn (i, &lvcolumn); 
	}
	
	//
	// Store the initial sizes of some controls:
	m_cButtonRecover.GetWindowRect (m_rcInitialButtonRecover);
	ScreenToClient (m_rcInitialButtonRecover);
	m_cButtonRedo.GetWindowRect (m_rcInitialButtonRedo);
	ScreenToClient (m_rcInitialButtonRedo);
	m_cStaticTxTitle.GetWindowRect (m_rcInitialStaticTransactionTitle);
	ScreenToClient (m_rcInitialStaticTransactionTitle);
	m_cListCtrl1.GetWindowRect (m_rcInitialListCtrl1);
	ScreenToClient (m_rcInitialListCtrl1);
	m_cListCtrl2.GetWindowRect (m_rcInitialListCtrl2);
	ScreenToClient (m_rcInitialListCtrl2);

	m_sizeMargin.cx = m_rcInitialListCtrl2.left;
	m_sizeMargin.cy = m_rcInitialListCtrl2.left;
	m_cListCtrl2.SetAllowSelect(!m_bCheckGroupByTransaction);
	EnableButtons();
	EnableCheckPoint();

	m_cDatePickerAfter.SetFormat (_T("dd'-'MMM'-'yyy':'HH':'mm':'ss"));
	m_cDatePickerBefore.SetFormat(_T("dd'-'MMM'-'yyy':'HH':'mm':'ss"));
	m_cDatePickerAfter.SendMessage (DTM_SETSYSTEMTIME, GDT_NONE , 0);
	m_cDatePickerBefore.SendMessage (DTM_SETSYSTEMTIME, GDT_NONE , 0);

#if defined (SORT_INDICATOR)
	CHeaderCtrl* pHdrCtrl= m_cListCtrl1.GetHeaderCtrl();
	if (pHdrCtrl)
	{
		int i, nHeaderCount = pHdrCtrl->GetItemCount();
		pHdrCtrl->SetImageList(&m_ImageListOrder);
		HD_ITEM curItem;
		for (i=0; i<nHeader; i++)
		{
			pHdrCtrl->GetItem(i, &curItem);
			curItem.mask= HDI_IMAGE | HDI_FORMAT;
			curItem.iImage= -1;
			curItem.fmt= HDF_LEFT | HDF_IMAGE | HDF_STRING /*|HDF_BITMAP_ON_RIGHT*/;
			pHdrCtrl->SetItem(i, &curItem);
		}
	}

	pHdrCtrl= m_cListCtrl2.GetHeaderCtrl();
	if (pHdrCtrl)
	{
		int i, nHeaderCount = pHdrCtrl->GetItemCount();
		pHdrCtrl->SetImageList(&m_ImageListOrder);
		HD_ITEM curItem;
		for (i=0; i<nHeader2; i++)
		{
			pHdrCtrl->GetItem(i, &curItem);
			curItem.mask= HDI_IMAGE | HDI_FORMAT;
			curItem.iImage= -1;
			curItem.fmt= HDF_LEFT | HDF_IMAGE | HDF_STRING /*|HDF_BITMAP_ON_RIGHT*/;
			pHdrCtrl->SetItem(i, &curItem);
		}
	}
#endif // SORT_INDICATOR

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIjaDatabase::OnButtonView() 
{
	CWaitCursor doWaitCursor;
	try
	{
		m_cStaticTxTitle.SetWindowText(_T(""));
		CleanListCtrl1();
		CleanListCtrl2();

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

		CString strConnectedUser;
		CString strDatabase;
		CString strDatabaseOwner;
		m_queryTransactionInfo.GetDatabase (strDatabase, strDatabaseOwner);
		m_bInconsistent = FALSE;
		try
		{
			CaTemporarySession checkinconsistent (m_queryTransactionInfo.GetNode(), strDatabase);
		}
		catch (CeSqlException e)
		{
			CString strError = e.GetReason();
			CString strErrorText = _T("E_US0026 Database is inconsistent");
			if (strError.GetLength() >= strErrorText.GetLength())
			{
				strError = strError.Left(strErrorText.GetLength());
				if (strError.CompareNoCase(strErrorText) == 0)
				{
					m_bInconsistent = TRUE;
				}
			}
		}

		CString strDb2 = strDatabase;
		if (m_bInconsistent)
		{
			strDb2 = _T("iidbdb");
		}
		m_queryTransactionInfo.SetBeforeAfter (m_strBefore, m_strAfter);
		m_queryTransactionInfo.SetUser(m_strUser);
		m_queryTransactionInfo.SetInconsistent(m_bInconsistent);
		m_queryTransactionInfo.SetWait(m_bWait);
		m_queryTransactionInfo.SetAfterCheckPoint (m_bAfterCheckPoint);
		m_queryTransactionInfo.SetCheckPointNumber (_ttoi (m_strCheckPointNo));
		m_queryTransactionInfo.SetAbortedTransaction (m_bAbortedTransaction);
		m_queryTransactionInfo.SetCommittedTransaction (m_bCommittedTransaction);


		//
		// Opens the session
		CaTemporarySession session (m_queryTransactionInfo.GetNode(), strDb2);
		//
		// Get the connected user string. This function will throw exception if failed:
		IJA_QueryUserInCurSession (strConnectedUser);
		m_queryTransactionInfo.SetConnectedUser(strConnectedUser);
		session.Release();
		if (m_bInconsistent)
		{
			//
			// The Database is inconsistent.\nContinue anyhow ?
			CString strMsg;
			strMsg.LoadString(IDS_VIEW_DATABASE_INCONSISTENT);
			
			int nAnswer = AfxMessageBox (strMsg ,MB_ICONQUESTION|MB_YESNO);
			if (nAnswer != IDYES)
			{
				RefreshPaneDatabase(m_queryTransactionInfo.GetNode(), strDatabase, strDatabaseOwner);
				return;
			}
		}

		if (!m_bViewButtonClicked)
		{
			m_bViewButtonClicked = TRUE;
			m_cButtonSelectAll.ShowWindow (SW_SHOW);
			m_cButtonRedo.ShowWindow (SW_SHOW);
			m_cButtonRecover.ShowWindow (SW_SHOW);
			m_cListCtrl1.ShowWindow (SW_SHOW);
			m_cListCtrl2.ShowWindow (SW_SHOW);
			m_cCheckGroupByTransaction.ShowWindow (SW_SHOW);
			m_cStaticTxTitle.ShowWindow (SW_SHOW);
		}
		m_strInitialUser = m_strUser;

		BOOL bOk = IJA_QueryDatabaseTransaction (&m_queryTransactionInfo, m_listMainTrans);
		if (bOk)
		{
			//
			// DIsplay Transactions:
			DisplayMainTransaction();
			DisplayData();
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
		AfxMessageBox (_T("System error, CuDlgIjaDatabase::OnButtonView, query data failed"));
	}
}

void CuDlgIjaDatabase::GetSelectedTransactions (CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr)
{
	int i, nCount;
	int nFound = -1;
	CaDatabaseTransactionItemDataDetail* pItemDetail= NULL;
	CaDatabaseTransactionItemData* pItem = NULL;
	if (m_bCheckGroupByTransaction)
	{
		nCount = m_cListCtrl1.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListCtrl1.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
			{
				pItem = (CaDatabaseTransactionItemData*)m_cListCtrl1.GetItemData (i);
				if (!pItem)
					continue;
				CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction = pItem->GetDetailTransaction();
				POSITION pos = listTransaction.GetHeadPosition();
				while (pos != NULL)
				{
					CaDatabaseTransactionItemDataDetail* pTr = listTransaction.GetNext(pos);
					pTr->SetLsnEnd (pItem->GetLsnEnd());
					ltr.AddTail (pTr);
				}
			}
		}
	}
	else
	{
		//
		// Get the selected Transaction IDs (no duplicated) from the detail list:
		CString strTransID;
		CStringList listTransID;
		nCount = m_cListCtrl2.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			if (m_cListCtrl2.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
			{
				pItemDetail= (CaDatabaseTransactionItemDataDetail*)m_cListCtrl2.GetItemData (i);
				if (!pItemDetail)
					continue;
				if (pItemDetail->IsBeginEnd())
					continue;
				strTransID = pItemDetail->GetStrTransactionID();
				if (ExistTransactionID(listTransID, strTransID))
					continue;
				listTransID.AddTail (strTransID);
			}
		}
		//
		// For each transaction ID of Main list, get all the detail Trans:
		LVFINDINFO lvfindinfo;
		memset (&lvfindinfo, 0, sizeof (LVFINDINFO));
		lvfindinfo.flags = LVFI_STRING;

		POSITION pos = listTransID.GetHeadPosition();
		while (pos != NULL)
		{
			strTransID =listTransID.GetNext(pos);
			//
			// Find the Transaction ID in the Main list of Transaction:
			lvfindinfo.psz   = strTransID;
			nFound = m_cListCtrl1.FindItem (&lvfindinfo);
			ASSERT (nFound != -1);
			if (nFound != -1)
			{
				pItem = (CaDatabaseTransactionItemData*)m_cListCtrl1.GetItemData (nFound);
				if (!pItem)
					continue;
				CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction = pItem->GetDetailTransaction();
				POSITION p = listTransaction.GetHeadPosition();
				while (p != NULL)
				{
					CaDatabaseTransactionItemDataDetail* pTr = listTransaction.GetNext(p);
					pTr->SetLsnEnd (pItem->GetLsnEnd());
					ltr.AddTail (pTr);
				}
			}
		}
	}
}


void CuDlgIjaDatabase::OnButtonRecover() 
{
	CWaitCursor doWaitCursor;
	if (m_bInconsistent)
	{
		CString strMsg;
		strMsg.LoadString (IDS_RECOVERREDO_IMPOSSIBLE);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
		return;
	}

	CxDlgRecover dlg;
	CTypedPtrList < CObList, CaBaseTransactionItemData* >& ltr = dlg.GetListTransaction();
	//
	// Prepare the selected transactions:
	GetSelectedTransactions (ltr);
	if (!RCRD_CheckValidateTransaction (ltr))
		return;

	UINT* pArraySelectedItem = NULL;
	int   nSelectedCount = 0;
	GetSelectedItems (pArraySelectedItem, nSelectedCount);
	dlg.SetQueryTransactionInfo(&m_queryTransactionInfo);
	dlg.SetInitialSelection (pArraySelectedItem, nSelectedCount);
	//
	// Luanch the dialog box:
	dlg.DoModal();
	if (pArraySelectedItem)
		delete [] pArraySelectedItem;
}


void CuDlgIjaDatabase::OnButtonRedo() 
{
	CWaitCursor doWaitCursor;
	if (m_bInconsistent)
	{
		CString strMsg;
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

	UINT* pArraySelectedItem = NULL;
	int   nSelectedCount = 0;
	GetSelectedItems (pArraySelectedItem, nSelectedCount);
	dlg.SetQueryTransactionInfo(&m_queryTransactionInfo);
	dlg.SetInitialSelection (pArraySelectedItem, nSelectedCount);
	//
	// Luanch the dialog box:
	dlg.DoModal();
	if (pArraySelectedItem)
		delete [] pArraySelectedItem;

}

void CuDlgIjaDatabase::OnDestroy() 
{
	CleanListCtrl1();
	CleanListCtrl2();
	CDialog::OnDestroy();
}

void CuDlgIjaDatabase::CleanListCtrl1()
{
	m_cListCtrl1.DeleteAllItems();
	while (!m_listMainTrans.IsEmpty())
		delete m_listMainTrans.RemoveHead();
}

void CuDlgIjaDatabase::CleanListCtrl2()
{
	m_cListCtrl2.DeleteAllItems();
}

void CuDlgIjaDatabase::DisplayData()
{
	CaDatabaseTransactionItemData* pItem = NULL;
	CleanListCtrl2();
	int i, nCount = m_cListCtrl1.GetItemCount();
	int nSelCount = m_cListCtrl1.GetSelectedCount();
	
	if (nSelCount > 1 && m_bCheckGroupByTransaction)
	{
		m_strTxTitle.LoadString (IDS_DETAIL_TRANSACTION2);
		UpdateData(FALSE);
	}
	
	if (m_bCheckGroupByTransaction)
	{
		//
		// GROUP BY TRANSACTION: display the detail for those items that are selected ONLY:
		int nSel = -1;
		for (i=0; i<nCount; i++)
		{
			if (m_cListCtrl1.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
			{
				nSel = i;
				pItem = (CaDatabaseTransactionItemData*)m_cListCtrl1.GetItemData (i);
				DisplayDetailItem (pItem);
			}
		}
	}
	else
	{
		//
		// ALL TRANSACTIONS are displayed !!!:
		for (i=0; i<nCount; i++)
		{
			pItem = (CaDatabaseTransactionItemData*)m_cListCtrl1.GetItemData (i);
			DisplayDetailItem (pItem);
		}
	}

	if (nSelCount == 1 && pItem && m_bCheckGroupByTransaction)
	{
		AfxFormatString1 (m_strTxTitle, IDS_DETAIL_TRANSACTION1, (LPCTSTR)pItem->GetStrTransactionID());
		UpdateData(FALSE);
	}
}

void CuDlgIjaDatabase::DisplayDetailItem (CaDatabaseTransactionItemData* pItem)
{
	CString strTable;
	if (!pItem)
		return;
	CTypedPtrList < CObList, CaDatabaseTransactionItemDataDetail* >& listTransaction = pItem->GetDetailTransaction();
	//
	// DIsplay the Detail Transactions:
	int idx, nCount = 0;
	int nImage = -1;

	if (!m_bCheckGroupByTransaction)
	{
		CaDatabaseTransactionItemDataDetail& begin = pItem->GetBeginTransaction();
		nImage = begin.GetImageId();
		nCount = m_cListCtrl2.GetItemCount();
		idx = m_cListCtrl2.InsertItem (nCount, _T(""), nImage);
		if (idx != -1)
		{
			m_cListCtrl2.SetItemData (idx, (DWORD)&begin);
			m_cListCtrl2.SetItemText (idx, 0, begin.GetStrTransactionID());
			m_cListCtrl2.SetItemText (idx, 1, begin.GetLsn());
			m_cListCtrl2.SetItemText (idx, 2, begin.GetUser());
			m_cListCtrl2.SetItemText (idx, 3, begin.GetStrAction());
			m_cListCtrl2.SetItemText (idx, 4, _T(""));
			m_cListCtrl2.SetItemText (idx, 5, _T(""));
		}
	}

	POSITION pos = listTransaction.GetHeadPosition();
	while (pos !=  NULL)
	{
		CaDatabaseTransactionItemDataDetail* pItemDetail = listTransaction.GetNext (pos);
		nCount = m_cListCtrl2.GetItemCount();
		nImage = pItemDetail->GetImageId();

		idx = m_cListCtrl2.InsertItem (nCount, _T(""), nImage);
		if (idx != -1)
		{
			switch (pItemDetail->GetAction())
			{
			case T_DELETE:
				IJA_ColorItem(&m_cListCtrl2, idx, theApp.m_property.TRGB_DELETE());
				break;
			case T_INSERT:
				IJA_ColorItem(&m_cListCtrl2, idx, theApp.m_property.TRGB_INSERT());
				break;
			case T_BEFOREUPDATE:
				IJA_ColorItem(&m_cListCtrl2, idx, theApp.m_property.TRGB_BEFOREUPDATE());
				break;
			case T_AFTERUPDATE:
				IJA_ColorItem(&m_cListCtrl2, idx, theApp.m_property.TRGB_AFTEREUPDATE());
				break;
			case T_CREATETABLE:
			case T_ALTERTABLE:
			case T_DROPTABLE:
			case T_RELOCATE:
			case T_MODIFY:
			case T_INDEX:
				IJA_ColorItem(&m_cListCtrl2, idx, theApp.m_property.TRGB_OTHERS());
				break;
			default:
				break;
			}
			AfxFormatString2 (strTable, IDS_OWNERxITEM, (LPCTSTR)pItemDetail->GetTableOwner(), (LPCTSTR)pItemDetail->GetTable());
			m_cListCtrl2.SetItemData (idx, (DWORD)pItemDetail);
			m_cListCtrl2.SetItemText (idx, 0, pItemDetail->GetStrTransactionID());
			m_cListCtrl2.SetItemText (idx, 1, pItemDetail->GetLsn());
			m_cListCtrl2.SetItemText (idx, 2, pItemDetail->GetUser());
			m_cListCtrl2.SetItemText (idx, 3, pItemDetail->GetStrAction());
			m_cListCtrl2.SetItemText (idx, 4, strTable);
			m_cListCtrl2.SetItemText (idx, 5, pItemDetail->GetData());
		}
	}

	if (!m_bCheckGroupByTransaction)
	{
		CaDatabaseTransactionItemDataDetail& end = pItem->GetEndTransaction();
		nImage = end.GetImageId();
		nCount = m_cListCtrl2.GetItemCount();
		idx = m_cListCtrl2.InsertItem (nCount, _T(""), nImage);
		if (idx != -1)
		{
			m_cListCtrl2.SetItemData (idx, (DWORD)&end);
			m_cListCtrl2.SetItemText (idx, 0, end.GetStrTransactionID());
			m_cListCtrl2.SetItemText (idx, 1, end.GetLsn());
			m_cListCtrl2.SetItemText (idx, 2, end.GetUser());
			m_cListCtrl2.SetItemText (idx, 3, end.GetStrAction());
			m_cListCtrl2.SetItemText (idx, 4, _T(""));
			m_cListCtrl2.SetItemText (idx, 5, _T(""));
		}
	}
	else
	{
		m_cStaticTxTitle.SetWindowText (m_strTxTitle);
	}
}


void CuDlgIjaDatabase::ResizeControls()
{
	if (!IsWindow(m_cListCtrl1.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);

	if (m_bCheckGroupByTransaction)
	{
		r = m_rcInitialListCtrl1;
		r.right  = rDlg.right  - r.left;
		m_cListCtrl1.MoveWindow (r);

		if (m_bViewButtonClicked)
			m_cListCtrl1.ShowWindow(SW_SHOW);

		r = m_rcInitialButtonRedo;
		r.right = rDlg.right  - m_sizeMargin.cx;
		r.left  = r.right - m_rcInitialButtonRedo.Width();
		m_cButtonRedo.MoveWindow (r);
		
		int nX1 = r.left;
		r = m_rcInitialButtonRecover;
		r.right = nX1 - 1;
		r.left  = r.right - m_rcInitialButtonRecover.Width();
		m_cButtonRecover.MoveWindow (r);

		if (m_bViewButtonClicked)
			m_cStaticTxTitle.ShowWindow (SW_SHOW);

		r = m_rcInitialListCtrl2;
		r.right  = rDlg.right   - r.left;
		r.bottom = rDlg.bottom  - r.left;
		m_cListCtrl2.MoveWindow (r);
		if (m_bViewButtonClicked)
		{
			m_cListCtrl2.ShowWindow (SW_SHOW);
			m_cListCtrl2.Invalidate();
		}
	}
	else
	{
		m_cListCtrl1.ShowWindow (SW_HIDE);
		m_cStaticTxTitle.ShowWindow (SW_HIDE);

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
		r = m_rcInitialListCtrl1;
		r.right  = rDlg.right  - r.left;
		r.bottom = nY1 - 4;
		m_cListCtrl2.MoveWindow (r);
		if (m_bViewButtonClicked)
		{
			m_cListCtrl2.ShowWindow (SW_SHOW);
			m_cListCtrl2.Invalidate();
		}
	}
}

void CuDlgIjaDatabase::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	int nSelCount = m_cListCtrl1.GetSelectedCount();
	if (nSelCount == 1)
	{
		int nSelected = m_cListCtrl1.GetNextItem(-1, LVNI_SELECTED);
		if (nSelected != -1)
		{
			CaDatabaseTransactionItemData* pItem = (CaDatabaseTransactionItemData*)m_cListCtrl1.GetItemData (nSelected);
			if (pItem)
			{
				CleanListCtrl2();
				AfxFormatString1 (m_strTxTitle, IDS_DETAIL_TRANSACTION1, (LPCTSTR)pItem->GetStrTransactionID());
				DisplayDetailItem (pItem);
			}
		}
	}
	else
	if (nSelCount > 1)
	{
		CleanListCtrl2();
		if (m_bCheckGroupByTransaction)
			DisplayData();
	}
	else
	{
		CleanListCtrl2();
	}
	EnableButtons();
}

void CuDlgIjaDatabase::OnCheckGroupByTransaction() 
{
	m_cStaticTxTitle.SetWindowText(_T(""));
	UpdateData (TRUE);
	m_cListCtrl2.SetAllowSelect(!m_bCheckGroupByTransaction);
	ResizeControls();
	DisplayData();
}

void CuDlgIjaDatabase::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

void CuDlgIjaDatabase::OnButtonSelectAll() 
{
	CleanListCtrl2();
	UpdateData (TRUE);
	m_bSelectAll = TRUE;

	if (m_bCheckGroupByTransaction)
	{
		int i, nCount = m_cListCtrl1.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			m_cListCtrl1.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	else
	{
		DisplayData();
		int i, nCount = m_cListCtrl2.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			m_cListCtrl2.SetItemState (i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	m_bSelectAll = FALSE;

	if (m_bCheckGroupByTransaction)
		DisplayData();
}


LONG CuDlgIjaDatabase::OnListCtrl1KeyUp(WPARAM w, LPARAM l)
{
	if ((CuListCtrl*)w == &m_cListCtrl1)
	{
		DisplayData();
		EnableButtons();
	}
	return 0;
}

void CuDlgIjaDatabase::GetSelectedItems (UINT*& pArraySelectedItem, int& nSelectedCount)
{
	try
	{
		int i, nCount = m_cListCtrl2.GetItemCount();
		int idx = 0, nSelCount = 0;

		if (m_bCheckGroupByTransaction)
		{
			if (nCount == 0)
				return;
			nSelectedCount = nCount;
			pArraySelectedItem = new UINT [nCount];
			memset (pArraySelectedItem, 0, (UINT)nCount);
			for (i=0; i<nCount; i++)
			{
				pArraySelectedItem[idx] = (UINT)m_cListCtrl2.GetItemData(i);
				idx++;
			}
		}
		else
		{
			nSelCount = m_cListCtrl2.GetSelectedCount();
			if (nSelCount == 0)
				return;
			nSelectedCount = 0;
			pArraySelectedItem = new UINT [nSelCount];
			memset (pArraySelectedItem, 0, (UINT)nSelCount);
			for (i=0; i<nCount; i++)
			{
				if (m_cListCtrl2.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
				{
					CaDatabaseTransactionItemDataDetail* pSel = (CaDatabaseTransactionItemDataDetail*)m_cListCtrl2.GetItemData(i);
					if (!pSel)
						continue;
					if (pSel->IsBeginEnd())
						continue;
					pArraySelectedItem[idx] = (UINT)pSel;
					idx++;
					nSelectedCount = idx;
				}
			}
		}
	}
	catch (...)
	{
		TRACE0 ("CuDlgIjaDatabase::GetSelectedItems \n");
	}
}

void CuDlgIjaDatabase::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	if (pNMListView->iSubItem == -1)
		return;

	CWaitCursor doWaitCursor;
	if (m_sortparam1.m_nItem == pNMListView->iSubItem)
	{
		m_sortparam1.m_bAsc = !m_sortparam1.m_bAsc;
	}
	else
		m_sortparam1.m_bAsc = TRUE;
	m_sortparam1.m_nItem = pNMListView->iSubItem;

	m_cListCtrl1.SortItems(CompareSubItem1, (LPARAM)&m_sortparam1);
	ChangeSortIndicator(&m_cListCtrl1, &m_sortparam1);
}

void CuDlgIjaDatabase::OnColumnclickList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	if (pNMListView->iSubItem == -1)
		return;

	CWaitCursor doWaitCursor;
	if (m_sortparam2.m_nItem == pNMListView->iSubItem)
	{
		m_sortparam2.m_bAsc = !m_sortparam2.m_bAsc;
	}
	else
		m_sortparam2.m_bAsc = TRUE;
	m_sortparam2.m_nItem = pNMListView->iSubItem;

	m_cListCtrl2.SortItems(CompareSubItem2, (LPARAM)&m_sortparam2);
	ChangeSortIndicator(&m_cListCtrl2, &m_sortparam2);
}


static int CALLBACK CompareSubItem1 (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC;
	CaListCtrlColorItemData* lpItem1 = (CaListCtrlColorItemData*)lParam1;
	CaListCtrlColorItemData* lpItem2 = (CaListCtrlColorItemData*)lParam2;
	if (lpItem1 == NULL || lpItem2 == NULL)
		return 0;
	CaDatabaseTransactionItemData* pItem1 = (CaDatabaseTransactionItemData*)lpItem1->m_lUserData;
	CaDatabaseTransactionItemData* pItem2 = (CaDatabaseTransactionItemData*)lpItem2->m_lUserData;

	SORTPARAMS*   pSr = (SORTPARAMS*)lParamSort;
	BOOL bAsc = pSr? pSr->m_bAsc: TRUE;

	if (pItem1 && pItem2)
	{
		nC = bAsc? 1 : -1;
		return nC * pItem1->Compare (pItem2, pSr->m_nItem);
	}
	else
	{
		nC = pItem1? 1: pItem2? -1: 0;
		return bAsc? nC: (-1)*nC;
	}

	return 0;
}

static int CALLBACK CompareSubItem2 (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC;
	CaListCtrlColorItemData* lpItem1 = (CaListCtrlColorItemData*)lParam1;
	CaListCtrlColorItemData* lpItem2 = (CaListCtrlColorItemData*)lParam2;
	if (lpItem1 == NULL || lpItem2 == NULL)
		return 0;
	CaDatabaseTransactionItemDataDetail* pItem1 = (CaDatabaseTransactionItemDataDetail*)lpItem1->m_lUserData;
	CaDatabaseTransactionItemDataDetail* pItem2 = (CaDatabaseTransactionItemDataDetail*)lpItem2->m_lUserData;

	SORTPARAMS*   pSr = (SORTPARAMS*)lParamSort;
	BOOL bAsc = pSr? pSr->m_bAsc: TRUE;

	if (pItem1 && pItem2)
	{
		nC = bAsc? 1 : -1;
		return nC * pItem1->Compare (pItem2, pSr->m_nItem);
	}
	else
	{
		nC = pItem1? 1: pItem2? -1: 0;
		return bAsc? nC: (-1)*nC;
	}

	return 0;
}

void CuDlgIjaDatabase::EnableButtons()
{
	int  nSelCount = m_bCheckGroupByTransaction? m_cListCtrl1.GetSelectedCount(): m_cListCtrl2.GetSelectedCount();
	BOOL bEnable = (nSelCount > 0 && m_bEnableRecoverRedo)? TRUE: FALSE;
	m_cButtonRedo.EnableWindow (bEnable);
	m_cButtonRecover.EnableWindow (bEnable);
}

void CuDlgIjaDatabase::OnItemchangedList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	EnableButtons();
}

void CuDlgIjaDatabase::ChangeSortIndicator(CListCtrl* pListCtrl, SORTPARAMS* pSp)
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
	
	pHdrCtrl->GetItem(pSp->m_nItem, &curItem);
	curItem.iImage = pSp->m_bAsc? IM_HEADER_DOWN: IM_HEADER_UP;
	pHdrCtrl->SetItem(pSp->m_nItem, &curItem);
#endif // SORT_INDICATOR
}

void CuDlgIjaDatabase::OnButtonMoveLog2Journals() 
{
	TRACE0 ("CuDlgIjaDatabase::OnButtonMoveLog2Journals() \n");
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

		session.Release (SESSION_COMMIT);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error: CuDlgIjaDatabase::OnButtonMoveLog2Journals failed"), MB_OK|MB_ICONEXCLAMATION);
	}
}

void CuDlgIjaDatabase::OnItemchangingList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	if (pNMListView->iItem >= 0 && pNMListView->uNewState > 0 )
	{
		int nSelected = pNMListView->iItem;
		CaDatabaseTransactionItemDataDetail* pItem = (CaDatabaseTransactionItemDataDetail*)m_cListCtrl2.GetItemData (nSelected);
		if (!pItem)
			return;
		if (pItem->IsBeginEnd())
		{
			*pResult = 1;
		}
	}
}

void CuDlgIjaDatabase::EnableCheckPoint()
{
	BOOL bCheck = (m_cCheckAfterCheckPoint.GetCheck() == 1)? TRUE: FALSE;
	m_cEditAfterCheckPoint.EnableWindow (bCheck);
	m_checkpoint_list_button.EnableWindow (bCheck);
}

void CuDlgIjaDatabase::OnCheckAfterCheckPoint() 
{
	EnableCheckPoint();
	OnInputParamChange(IDCT_CHECK_CHECKPOINT);
}

void CuDlgIjaDatabase::OnCheckAbortedTransaction() 
{
	OnInputParamChange(IDCT_ABORT_TRANS);
}

void CuDlgIjaDatabase::OnCheckCommittedTransaction() 
{
	OnInputParamChange(IDCT_COMMIT_TRANS);
}

void CuDlgIjaDatabase::OnSelchangeComboUser() 
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


void CuDlgIjaDatabase::OnDatetimechangeBefore(NMHDR* pNMHDR, LRESULT* pResult) 
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
		TRACE1 ("CuDlgIjaDatabase::OnDatetimechangeBefore...New time = %s \n", strTm);
		OnInputParamChange(IDCT_BEFOR);
	}
}


void CuDlgIjaDatabase::OnDatetimechangeAfter(NMHDR* pNMHDR, LRESULT* pResult) 
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
		TRACE1 ("CuDlgIjaDatabase::OnDatetimechangeAfter...New time = %s \n", strTm);
		OnInputParamChange(IDCT_AFTER);
	}
}

void CuDlgIjaDatabase::OnChangeEditAfterCheckPoint() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	OnInputParamChange(IDCT_EDIT_CHECKPOINT);
}

void CuDlgIjaDatabase::OnCheckWait() 
{
	OnInputParamChange(IDCT_WAIT);
}


void CuDlgIjaDatabase::OnInputParamChange(int idct)
{
	CString strNode = m_queryTransactionInfo.GetNode();
	CString strDatabase;
	CString strDatabaseOwner;
	m_queryTransactionInfo.GetDatabase (strDatabase, strDatabaseOwner);

	switch (idct)
	{
	case IDCT_AFTER:
	case IDCT_BEFOR:
	case IDCT_CHECK_CHECKPOINT:
	case IDCT_EDIT_CHECKPOINT:
	case IDCT_WAIT:
		if (strNode.IsEmpty() || strDatabase.IsEmpty())
			break;
		RefreshPaneDatabase(strNode, strDatabase, strDatabaseOwner);
		break;

	case IDCT_ABORT_TRANS:
	case IDCT_COMMIT_TRANS:
	case IDCT_USER:
		UpdateData (TRUE);
		m_cListCtrl1.DeleteAllItems();
		m_cListCtrl2.DeleteAllItems();
		DisplayMainTransaction();
		DisplayData();
		EnableButtons();
		EnableCheckPoint();
		break;
	default:
		if (strNode.IsEmpty() || strDatabase.IsEmpty())
			break;
		RefreshPaneDatabase(strNode, strDatabase, strDatabaseOwner);
		break;
	}

	TRACE0("CuDlgIjaDatabase::OnInputParamChange() \n");
}

void CuDlgIjaDatabase::DisplayMainTransaction()
{
	//
	// DIsplay Transactions:
	int idx, nCount = 0, nImage = 0;
	POSITION pos = m_listMainTrans.GetHeadPosition();
	while (pos !=  NULL)
	{
		CaDatabaseTransactionItemData* pItem = m_listMainTrans.GetNext (pos);
		ASSERT (pItem);
		if (!pItem)
			continue;
		BOOL bCommit = pItem->GetCommit();
		BOOL bAbort  = !bCommit;

		if (!(bCommit && m_bCommittedTransaction) && !(bAbort && m_bAbortedTransaction))
			continue;

		if (m_strUser.CompareNoCase(theApp.m_strAllUser) != 0 && m_strUser.CompareNoCase (pItem->GetUser()) != 0)
			continue;

		nImage = pItem->GetImageId();
		nCount = m_cListCtrl1.GetItemCount();
		idx = m_cListCtrl1.InsertItem (nCount, _T(""), nImage);
		if (idx != -1)
		{
			m_cListCtrl1.SetItemData (idx, (DWORD)pItem);

			m_cListCtrl1.SetItemText (idx, 0, pItem->GetStrTransactionID());
			m_cListCtrl1.SetItemText (idx, 1, pItem->GetUser());
			m_cListCtrl1.SetItemText (idx, 2, pItem->GetBeginLocale());
			m_cListCtrl1.SetItemText (idx, 3, pItem->GetEndLocale());
			m_cListCtrl1.SetItemText (idx, 4, pItem->GetDuration());

			m_cListCtrl1.SetItemText (idx, 5, pItem->GetStrCountInsert());
			m_cListCtrl1.SetItemText (idx, 6, pItem->GetStrCountUpdate());
			m_cListCtrl1.SetItemText (idx, 7, pItem->GetStrCountDelete());
			m_cListCtrl1.SetItemText (idx, 8, pItem->GetStrCountTotal());
		}
	}
}

void CuDlgIjaDatabase::OnKillfocusEditAfterCheckPoint() 
{
	CString m_strText;
	m_cEditAfterCheckPoint.GetWindowText(m_strText);
	m_strText.TrimLeft();
	m_strText.TrimRight();

	int nVal = _ttoi(m_strText);
	if (nVal <= 0)
		m_cEditAfterCheckPoint.SetWindowText(_T("1"));
}


void CuDlgIjaDatabase::OnCheckpointListButton() 
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
