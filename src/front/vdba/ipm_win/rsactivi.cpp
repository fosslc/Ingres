/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/


/*
**  Project  : Visual DBA
**  Source   : rsactivi.cpp  Implementation file
**  Author   : UK Sotheavut 
**  Purpose  : Page of a static item Replication.  (Activity)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created.
** 05-May-1999 (noifr01)
**    fixed bug #96842 : replic queues were not refreshed through
**    force refresh or backgorund refresh
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmprdta.h"
#include "rsactivi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationStaticPageActivity::CuDlgReplicationStaticPageActivity(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationStaticPageActivity::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationStaticPageActivity)
	m_strInputQueue = _T("");
	m_strDistributionQueue = _T("");
	m_strStartingTime = _T("");
	//}}AFX_DATA_INIT
	m_pCurrentPage      = NULL;

	m_pDatabaseOutgoing = NULL;
	m_pDatabaseIncoming = NULL;
	m_pDatabaseTotal    = NULL;

	m_pTableOutgoing    = NULL;
	m_pTableIncoming    = NULL;
	m_pTableTotal       = NULL;
}


void CuDlgReplicationStaticPageActivity::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationStaticPageActivity)
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonNow);
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Text(pDX, IDC_EDIT1, m_strInputQueue);
	DDX_Text(pDX, IDC_EDIT2, m_strDistributionQueue);
	DDX_Text(pDX, IDC_EDIT3, m_strStartingTime);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationStaticPageActivity, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationStaticPageActivity)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonNow)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_DBEVENT_TRACE_INCOMING, OnDbeventIncoming)
	ON_MESSAGE (WM_USER_IPMPAGE_LEAVINGPAGE, OnLeavingPage)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()



void CuDlgReplicationStaticPageActivity::UpdateSummary (int iColumn, int nMode)
{
	enum {SU_INSERT = 1, SU_UPDATE, SU_DELETE, SU_TOTAL, SU_TRANSACTION};
	LONG    lNumber, lTotal = 0;
	CString strItem;
	//
	// Increase by +1 of item (Incomming or Outgoing) of the type
	// SU_INSERT, or SU_UPDATE, or SU_DELETE, or SU_TRANSACTION:
	strItem = m_cListCtrl.GetItemText (nMode, iColumn);
	lNumber = _ttol ((LPCTSTR)strItem);
	lNumber++;
	strItem.Format (_T("%ld"), lNumber);
	m_cListCtrl.SetItemText (nMode, iColumn, strItem);
	//
	// Total = Sum (SU_INSERT, SU_UPDATE, SU_DELETE)
	// Do not include the SU_TRANSACTION:
	if (iColumn != SU_TRANSACTION)
	{
		strItem = m_cListCtrl.GetItemText (nMode, SU_TOTAL);
		lNumber = _ttol ((LPCTSTR)strItem);
		lNumber++;
		strItem.Format (_T("%ld"), lNumber);
		m_cListCtrl.SetItemText (nMode, SU_TOTAL, strItem);
	}
	//
	// Total by Type: (Incomming + Outgoing) of SU_INSERT, or SU_UPDATE, or SU_DELETE, or SU_TRANSACTION:
	strItem = m_cListCtrl.GetItemText (MODE_TOTAL, iColumn);
	lNumber = _ttol ((LPCTSTR)strItem);
	lNumber++;
	strItem.Format (_T("%ld"), lNumber);
	m_cListCtrl.SetItemText (MODE_TOTAL, iColumn, strItem);
	//
	// Add total of the sub-total:
	strItem = m_cListCtrl.GetItemText (MODE_TOTAL, SU_INSERT);
	lNumber = _ttol ((LPCTSTR)strItem);
	lTotal += lNumber;
	strItem = m_cListCtrl.GetItemText (MODE_TOTAL, SU_UPDATE);
	lNumber = _ttol ((LPCTSTR)strItem);
	lTotal += lNumber;
	strItem = m_cListCtrl.GetItemText (MODE_TOTAL, SU_DELETE);
	lNumber = _ttol ((LPCTSTR)strItem);
	lTotal += lNumber;
	strItem.Format (_T("%ld"), lTotal);
	m_cListCtrl.SetItemText (MODE_TOTAL, SU_TOTAL, strItem);
}



void CuDlgReplicationStaticPageActivity::UpdateDatabase (int iColumn, LPRAISEDREPLICDBE pData, int nMode)
{
	int iFound = -1;
	LVFINDINFO lvfind;
	CuListCtrl* pList = NULL;
	enum {DB_INSERT = 1, DB_UPDATE, DB_DELETE, DB_TOTAL, DB_TRANSACTION};
	LONG    lNumber;
	CString strItem;
	CString strDb;
	
	switch (nMode)
	{
	case MODE_TOTAL:
		pList = &(m_pDatabaseTotal->m_cListCtrl);
		break;
	case MODE_INCOMING:
		pList = &(m_pDatabaseIncoming->m_cListCtrl);
		break;
	case MODE_OUTGOING:
		pList = &(m_pDatabaseOutgoing->m_cListCtrl);
		break;
	default:
		pList = NULL;
		break;
	}
	ASSERT (pList);
	if (!pList)
		return;
	memset (&lvfind, 0, sizeof (lvfind));
	lvfind.flags = LVFI_STRING;
	lvfind.psz   = (LPTSTR)pData->DBName;
	iFound = pList->FindItem (&lvfind);
	if (iFound != -1)
	{
		//
		// Existing Database
		strItem = pList->GetItemText (iFound, iColumn);
		lNumber = _ttol ((LPCTSTR)strItem);
		lNumber++;
		strItem.Format (_T("%ld"), lNumber);
		pList->SetItemText (iFound, iColumn, strItem);

		//
		// Exclude the Transaction:
		if (iColumn != DB_TRANSACTION)
		{
			strItem = pList->GetItemText (iFound, DB_TOTAL);
			lNumber = _ttol ((LPCTSTR)strItem);
			lNumber++;
			strItem.Format (_T("%ld"), lNumber);
			pList->SetItemText (iFound, DB_TOTAL, strItem);
		}
	}
	else
	{
		//
		// New Database 
		int nNewItem = pList->InsertItem (pList->GetItemCount(), (LPCTSTR)pData->DBName);
		pList->SetItemText (nNewItem, DB_INSERT,      _T("0"));
		pList->SetItemText (nNewItem, DB_UPDATE,      _T("0"));
		pList->SetItemText (nNewItem, DB_DELETE,      _T("0"));
		if (iColumn != DB_TRANSACTION)
			pList->SetItemText (nNewItem, DB_TOTAL,   _T("1"));
		pList->SetItemText (nNewItem, DB_TRANSACTION, _T("0"));
		pList->SetItemText (nNewItem, iColumn       , _T("1"));
	}
}


void CuDlgReplicationStaticPageActivity::UpdateTable (int iColumn, LPRAISEDREPLICDBE pData, int nMode)
{
	int i, iFound = -1, nCount = 0;
	CuListCtrl* pList = NULL;
	enum {TB_DATABASE, TB_TABLE, TB_INSERT, TB_UPDATE, TB_DELETE, TB_TOTAL};
	LONG    lNumber;
	CString strItem;
	CString strDb;
	
	switch (nMode)
	{
	case MODE_TOTAL:
		pList = &(m_pTableTotal->m_cListCtrl);
		break;
	case MODE_INCOMING:
		pList = &(m_pTableIncoming->m_cListCtrl);
		break;
	case MODE_OUTGOING:
		pList = &(m_pTableOutgoing->m_cListCtrl);
		break;
	default:
		pList = NULL;
		break;
	}
	ASSERT (pList);
	if (!pList)
		return;

	nCount = pList->GetItemCount();
	iFound = -1;
	for (i=0; i<nCount; i++)
	{
		strItem = pList->GetItemText (i, 1);
		if (strItem.CompareNoCase ((LPCTSTR)pData->TableName) == 0)
		{
			strDb = pList->GetItemText (i, 0);
			if (strDb.CompareNoCase ((LPCTSTR)pData->DBName) == 0)
			{
				iFound = i;
				break;
			}
		}
	}
	if (iFound != -1)
	{
		//
		// Existing Table
		strItem = pList->GetItemText (iFound, iColumn);
		lNumber = _ttol ((LPCTSTR)strItem);
		lNumber++;
		strItem.Format (_T("%ld"), lNumber);
		pList->SetItemText (iFound, iColumn, strItem);

		strItem = pList->GetItemText (iFound, TB_TOTAL);
		lNumber = _ttol ((LPCTSTR)strItem);
		lNumber++;
		strItem.Format (_T("%ld"), lNumber);
		pList->SetItemText (iFound, TB_TOTAL, strItem);
	}
	else
	{
		//
		// New table
		int nNewItem = -1;
		CString strNewItem = (LPCTSTR)pData->DBName;
		strNewItem += (LPCTSTR)pData->TableName;
		iFound = -1;
		for (i=0; i<nCount; i++)
		{
			strDb   = pList->GetItemText (i, TB_DATABASE);
			strItem = pList->GetItemText (i, TB_TABLE);
			strDb  += strItem;
			if (strNewItem.CompareNoCase (strDb) >= 0)
				continue;
			else
			{
				iFound = i;
				break;
			}
		}
		if (iFound != -1)
			nNewItem = pList->InsertItem (iFound, (LPCTSTR)pData->DBName);
		else
			nNewItem = pList->InsertItem (pList->GetItemCount(), (LPCTSTR)pData->DBName);
		pList->SetItemText (nNewItem, TB_TABLE,       (LPCTSTR)pData->TableName);
		pList->SetItemText (nNewItem, TB_INSERT,      _T("0"));
		pList->SetItemText (nNewItem, TB_UPDATE,      _T("0"));
		pList->SetItemText (nNewItem, TB_DELETE,      _T("0"));
		pList->SetItemText (nNewItem, TB_TOTAL,       _T("1"));
		pList->SetItemText (nNewItem, iColumn       , _T("1"));
	}
}


void CuDlgReplicationStaticPageActivity::UpdateDbTotal (int iColumn, LPRAISEDREPLICDBE pData)
{
	int iFound = -1;
	LVFINDINFO lvfind;
	enum {DB_INSERT = 1, DB_UPDATE, DB_DELETE, DB_TOTAL, DB_TRANSACTION};
	LONG    lNumber;
	CString strItem;
	//
	// Update the Total per Database.
	CuListCtrl& listctrlTotalDb = m_pDatabaseTotal->m_cListCtrl;
	memset (&lvfind, 0, sizeof (lvfind));
	lvfind.flags = LVFI_STRING;
	lvfind.psz   = (LPTSTR)pData->DBName;
	iFound = listctrlTotalDb.FindItem (&lvfind);
	if (iFound != -1)
	{
		//
		// Existing Database
		strItem = listctrlTotalDb.GetItemText (iFound, iColumn);
		lNumber = _ttol ((LPCTSTR)strItem);
		lNumber++;
		strItem.Format (_T("%ld"), lNumber);
		listctrlTotalDb.SetItemText (iFound, iColumn, strItem);
		//
		// Exclude the Transaction:
		if (iColumn != DB_TRANSACTION)
		{
			strItem = listctrlTotalDb.GetItemText (iFound, DB_TOTAL);
			lNumber = _ttol ((LPCTSTR)strItem);
			lNumber++;
			strItem.Format (_T("%ld"), lNumber);
			listctrlTotalDb.SetItemText (iFound, DB_TOTAL, strItem);
		}
	}
	else
	{
		//
		// New Database
		int nNewItem = listctrlTotalDb.InsertItem (listctrlTotalDb.GetItemCount(), (LPCTSTR)pData->DBName);
		listctrlTotalDb.SetItemText (nNewItem, DB_INSERT,      _T("0"));
		listctrlTotalDb.SetItemText (nNewItem, DB_UPDATE,      _T("0"));
		listctrlTotalDb.SetItemText (nNewItem, DB_DELETE,      _T("0"));
		if (iColumn != DB_TRANSACTION)
			listctrlTotalDb.SetItemText (nNewItem, DB_TOTAL,   _T("1"));
		listctrlTotalDb.SetItemText (nNewItem, DB_TRANSACTION, _T("0"));
		listctrlTotalDb.SetItemText (nNewItem, iColumn       , _T("1"));
	}
}


void CuDlgReplicationStaticPageActivity::UpdateTbTotal (int iColumn, LPRAISEDREPLICDBE pData)
{
	int i, iFound = -1, nCount = 0;
	enum {TB_DATABASE, TB_TABLE, TB_INSERT, TB_UPDATE, TB_DELETE, TB_TOTAL};
	LONG    lNumber;
	CString strItem;
	CString strDb;

	//
	// Update the Total per Table.
	CuListCtrl& listctrlTotalTb = m_pTableTotal->m_cListCtrl;
	nCount = listctrlTotalTb.GetItemCount();
	iFound = -1;
	for (i=0; i<nCount; i++)
	{
		strItem = listctrlTotalTb.GetItemText (i, 1);
		if (strItem.CompareNoCase ((LPCTSTR)pData->TableName) == 0)
		{
			strDb = listctrlTotalTb.GetItemText (i, 0);
			if (strDb.CompareNoCase ((LPCTSTR)pData->DBName) == 0)
			{
				iFound = i;
				break;
			}
		}
	}
	if (iFound != -1)
	{
		//
		// Existing Table
		strItem = listctrlTotalTb.GetItemText (iFound, iColumn);
		lNumber = _ttol ((LPCTSTR)strItem);
		lNumber++;
		strItem.Format (_T("%ld"), lNumber);
		listctrlTotalTb.SetItemText (iFound, iColumn, strItem);

		strItem = listctrlTotalTb.GetItemText (iFound, TB_TOTAL);
		lNumber = _ttol ((LPCTSTR)strItem);
		lNumber++;
		strItem.Format (_T("%ld"), lNumber);
		listctrlTotalTb.SetItemText (iFound, TB_TOTAL, strItem);
	}
	else
	{
		//
		// New Table
		int nNewItem = -1;
		CString strNewItem = (LPCTSTR)pData->DBName;
		strNewItem += (LPCTSTR)pData->TableName;
		iFound = -1;
		for (i=0; i<nCount; i++)
		{
			strDb   = listctrlTotalTb.GetItemText (i, TB_DATABASE);
			strItem = listctrlTotalTb.GetItemText (i, TB_TABLE);
			strDb  += strItem;
			if (strNewItem.CompareNoCase (strDb) >= 0)
				continue;
			else
			{
				iFound = i;
				break;
			}
		}
		if (iFound != -1)
			nNewItem = listctrlTotalTb.InsertItem (iFound, (LPCTSTR)pData->DBName);
		else
			nNewItem = listctrlTotalTb.InsertItem (listctrlTotalTb.GetItemCount(), (LPCTSTR)pData->DBName);
		listctrlTotalTb.SetItemText (nNewItem, TB_TABLE ,       (LPCTSTR)pData->TableName);
		listctrlTotalTb.SetItemText (nNewItem, TB_INSERT,      _T("0"));
		listctrlTotalTb.SetItemText (nNewItem, TB_UPDATE,      _T("0"));
		listctrlTotalTb.SetItemText (nNewItem, TB_DELETE,      _T("0"));
		listctrlTotalTb.SetItemText (nNewItem, TB_TOTAL,       _T("1"));
		listctrlTotalTb.SetItemText (nNewItem, iColumn       , _T("1"));
	}
}





void CuDlgReplicationStaticPageActivity::UpdateOutgoing (LPRAISEDREPLICDBE pData)
{
	int iColumn   = -1;
	int iColumnDB = -1;
	int iColumnTB = -1;
	enum {SU_INSERT = 1, SU_UPDATE, SU_DELETE, SU_TOTAL, SU_TRANSACTION};
	enum {DB_INSERT = 1, DB_UPDATE, DB_DELETE, DB_TOTAL, DB_TRANSACTION};
	enum {TB_INSERT = 2, TB_UPDATE, TB_DELETE, TB_TOTAL};
	
	switch (pData->EventType)
	{
	case REPLIC_DBE_OUT_INSERT:
		iColumn  = SU_INSERT;
		iColumnDB= DB_INSERT;
		iColumnTB= TB_INSERT;
		break;
	case REPLIC_DBE_OUT_UPDATE:
		iColumn  = SU_UPDATE;
		iColumnDB= DB_UPDATE;
		iColumnTB= TB_UPDATE;
		break;
	case REPLIC_DBE_OUT_DELETE:
		iColumn  = SU_DELETE;
		iColumnDB= DB_DELETE;
		iColumnTB= TB_DELETE;
		break;
	case REPLIC_DBE_OUT_TRANSACTION:
		iColumn  = SU_TRANSACTION;
		iColumnDB= DB_TRANSACTION;
		break;
	default:
		iColumn = -1;
		break;
	}
	if (iColumn != -1 && iColumn != SU_TRANSACTION)
	{
		UpdateSummary  (iColumn,   MODE_OUTGOING);
		UpdateDatabase (iColumnDB, pData, MODE_OUTGOING);
		UpdateTable    (iColumnTB, pData, MODE_OUTGOING);
		UpdateDbTotal  (iColumnDB, pData);
		UpdateTbTotal  (iColumnTB, pData);
	}
	else
	if (iColumn == SU_TRANSACTION)
	{
		UpdateSummary  (iColumn,   MODE_OUTGOING);
		UpdateDatabase (iColumnDB, pData, MODE_OUTGOING);
		UpdateDbTotal  (iColumnDB, pData);
	}
}

void CuDlgReplicationStaticPageActivity::UpdateIncoming (LPRAISEDREPLICDBE pData)
{
	int iColumn   = -1;
	int iColumnDB = -1;
	int iColumnTB = -1;
	enum {SU_INSERT = 1, SU_UPDATE, SU_DELETE, SU_TOTAL, SU_TRANSACTION};
	enum {DB_INSERT = 1, DB_UPDATE, DB_DELETE, DB_TOTAL, DB_TRANSACTION};
	enum {TB_INSERT = 2, TB_UPDATE, TB_DELETE, TB_TOTAL};
	
	switch (pData->EventType)
	{
	case REPLIC_DBE_IN_INSERT:
		iColumn  = SU_INSERT;
		iColumnDB= DB_INSERT;
		iColumnTB= TB_INSERT;
		break;
	case REPLIC_DBE_IN_UPDATE:
		iColumn  = SU_UPDATE;
		iColumnDB= DB_UPDATE;
		iColumnTB= TB_UPDATE;
		break;
	case REPLIC_DBE_IN_DELETE:
		iColumn  = SU_DELETE;
		iColumnDB= DB_DELETE;
		iColumnTB= TB_DELETE;
		break;
	case REPLIC_DBE_IN_TRANSACTION:
		iColumn  = SU_TRANSACTION;
		iColumnDB= DB_TRANSACTION;
		break;
	default:
		iColumn = -1;
		break;
	}
	if (iColumn != -1 && iColumn != SU_TRANSACTION)
	{
		UpdateSummary  (iColumn,   MODE_INCOMING);	
		UpdateDatabase (iColumnDB, pData, MODE_INCOMING);
		UpdateTable    (iColumnTB, pData, MODE_INCOMING);
		UpdateDbTotal  (iColumnDB, pData);
		UpdateTbTotal  (iColumnTB, pData);
	}
	else
	if (iColumn == SU_TRANSACTION)
	{
		UpdateSummary  (iColumn,   MODE_INCOMING);	
		UpdateDatabase (iColumnDB, pData, MODE_INCOMING);
		UpdateDbTotal  (iColumnDB, pData);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationStaticPageActivity message handlers


LONG CuDlgReplicationStaticPageActivity::OnDbeventIncoming (WPARAM wParam, LPARAM lParam)
{
	LPRAISEDREPLICDBE pData = (LPRAISEDREPLICDBE)lParam;
	if (!pData)
		return 0;
	switch (pData->EventType)
	{
	case REPLIC_DBE_OUT_INSERT:
	case REPLIC_DBE_OUT_UPDATE:
	case REPLIC_DBE_OUT_DELETE:
	case REPLIC_DBE_OUT_TRANSACTION:
		UpdateOutgoing (pData);
		break;
	case REPLIC_DBE_IN_INSERT:
	case REPLIC_DBE_IN_UPDATE:
	case REPLIC_DBE_IN_DELETE:
	case REPLIC_DBE_IN_TRANSACTION:
		UpdateIncoming (pData);
		break;
	default:
		break;
	}

	return 0;
}


void CuDlgReplicationStaticPageActivity::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgReplicationStaticPageActivity::OnDestroy() 
{
	CDialog::OnDestroy();
}

BOOL CuDlgReplicationStaticPageActivity::OnInitDialog() 
{
	try
	{
		CDialog::OnInitDialog();
		VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
		//
		// Initalize the Column Header of CListCtrl (CuListCtrl)
		// If modify this constant and the column width, make sure do not forget
		// to port to the OnLoad() and OnGetData() members.
		const int LAYOUT_NUMBER = 6;
		LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
		{
			{IDS_TC_EMPTY,      120, LVCFMT_LEFT,  FALSE},
			{IDS_TC_INSERT,      60, LVCFMT_RIGHT, FALSE},
			{IDS_TC_UPDATE,      60, LVCFMT_RIGHT, FALSE},
			{IDS_TC_DELETE,      60, LVCFMT_RIGHT, FALSE},
			{IDS_TC_TOTAL,       60, LVCFMT_RIGHT, FALSE},
			{IDS_TC_TRANSACTION,100, LVCFMT_RIGHT, FALSE}
		};

		CString strOutGoing; //_T("Outgoing")
		CString strIncoming; //_T("Incoming")
		CString strTotal;    //_T("Total")
		strOutGoing.LoadString(IDS_TC_OUTGOING);
		strIncoming.LoadString(IDS_TC_INCOMING);
		strTotal.LoadString(IDS_TC_TOTAL);

		InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

		m_cListCtrl.InsertItem (0, strOutGoing);
		m_cListCtrl.InsertItem (1, strIncoming);
		m_cListCtrl.InsertItem (2, strTotal);
	
		m_pDatabaseOutgoing = new CuDlgReplicationStaticPageActivityPerDatabase (CuDlgReplicationStaticPageActivityPerDatabase::MODE_OUTGOING, &m_cTab1);
		m_pDatabaseIncoming = new CuDlgReplicationStaticPageActivityPerDatabase (CuDlgReplicationStaticPageActivityPerDatabase::MODE_INCOMING, &m_cTab1);
		m_pDatabaseTotal    = new CuDlgReplicationStaticPageActivityPerDatabase (CuDlgReplicationStaticPageActivityPerDatabase::MODE_TOTAL, &m_cTab1);
		
		m_pTableOutgoing    = new CuDlgReplicationStaticPageActivityPerTable    (CuDlgReplicationStaticPageActivityPerTable::MODE_OUTGOING, &m_cTab1);
		m_pTableIncoming    = new CuDlgReplicationStaticPageActivityPerTable    (CuDlgReplicationStaticPageActivityPerTable::MODE_INCOMING, &m_cTab1);
		m_pTableTotal       = new CuDlgReplicationStaticPageActivityPerTable    (CuDlgReplicationStaticPageActivityPerTable::MODE_TOTAL, &m_cTab1);

		m_pDatabaseOutgoing->Create (IDD_REPSTATIC_PAGE_ACTIVITY_TAB_DATABASE, &m_cTab1);
		m_pDatabaseIncoming->Create (IDD_REPSTATIC_PAGE_ACTIVITY_TAB_DATABASE, &m_cTab1);
		m_pDatabaseTotal->Create (IDD_REPSTATIC_PAGE_ACTIVITY_TAB_DATABASE, &m_cTab1);
		
		m_pTableOutgoing->Create (IDD_REPSTATIC_PAGE_ACTIVITY_TAB_TABLE, &m_cTab1);
		m_pTableIncoming->Create (IDD_REPSTATIC_PAGE_ACTIVITY_TAB_TABLE, &m_cTab1);
		m_pTableTotal->Create (IDD_REPSTATIC_PAGE_ACTIVITY_TAB_TABLE, &m_cTab1);

		//
		// Initalize the Tab Control.
		const int TAB_NUMBER = 6;
		TCITEM tcitem;
		UINT IdsTab[TAB_NUMBER] = 
		{
			IDS_TC_OUTGOING,
			IDS_TC_INCOMING,
			IDS_TC_TOTAL,

			IDS_TC_OUTGOING,
			IDS_TC_INCOMING,
			IDS_TC_TOTAL
		};
		CWnd* dlgArray[TAB_NUMBER] = 
		{
			(CWnd*)m_pDatabaseOutgoing,
			(CWnd*)m_pDatabaseIncoming,
			(CWnd*)m_pDatabaseTotal,
			(CWnd*)m_pTableOutgoing,
			(CWnd*)m_pTableIncoming,
			(CWnd*)m_pTableTotal
		};
		int nImage [TAB_NUMBER] = {0, 0, 0, 1, 1, 1};
		m_tabImageList.Create(IDB_DATABASEOBJECT, 16, 1, RGB(255, 0, 255));
		m_cTab1.SetImageList (&m_tabImageList);
		memset (&tcitem, 0, sizeof (tcitem));
		tcitem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
		tcitem.cchTextMax = 30;
		CString strHeader;
		for (int i=0; i<TAB_NUMBER; i++)
		{
			strHeader.LoadString(IdsTab[i]);
			tcitem.pszText = (LPTSTR)(LPCTSTR)strHeader;
			tcitem.iImage  = nImage[i];
			tcitem.lParam  = (LPARAM)dlgArray[i];
			m_cTab1.InsertItem (i, &tcitem); 
		}
		DisplayPage();
	}
	catch (...)
	{
		AfxMessageBox (IDS_E_PAGE_DETAIL);
		return FALSE;
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationStaticPageActivity::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);
	m_cListCtrl.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - r.left;
	m_cListCtrl.MoveWindow (r);

	m_cTab1.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right  - r.left;
	r.bottom = rDlg.bottom - r.left;
	m_cTab1.MoveWindow (r);

	if (m_pCurrentPage)
	{
		m_cTab1.GetClientRect (r);
		m_cTab1.AdjustRect (FALSE, r);
		m_pCurrentPage->MoveWindow(r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
	}
}

void CuDlgReplicationStaticPageActivity::OnButtonNow() 
{
	CTime t = CTime::GetCurrentTime();
	m_strStartingTime       = t.Format (_T("%c"));
	UpdateData (FALSE);
	for (int i=0; i<3; i++)
		for (int j=1; j<6; j++)
			m_cListCtrl.SetItemText (i, j, _T("0"));
	m_pDatabaseIncoming->m_cListCtrl.DeleteAllItems();
	m_pDatabaseOutgoing->m_cListCtrl.DeleteAllItems();
	m_pDatabaseTotal->m_cListCtrl.DeleteAllItems();
	m_pTableIncoming->m_cListCtrl.DeleteAllItems();
	m_pTableOutgoing->m_cListCtrl.DeleteAllItems();
	m_pTableTotal->m_cListCtrl.DeleteAllItems();}

void CuDlgReplicationStaticPageActivity::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	DisplayPage();
	*pResult = 0;
}


void CuDlgReplicationStaticPageActivity::DisplayPage(int nPage)
{
	if (nPage != -1)
		m_cTab1.SetCurSel (nPage);
	int nSel = m_cTab1.GetCurSel ();
	if (nSel == -1)
		return;

	TCITEM tcitem;
	memset (&tcitem, 0, sizeof (tcitem));
	tcitem.mask = TCIF_PARAM;

	m_cTab1.GetItem (nSel, &tcitem);
	if (tcitem.lParam)
	{
		if (m_pCurrentPage)
			m_pCurrentPage->ShowWindow (SW_HIDE);
		m_pCurrentPage = (CWnd*)tcitem.lParam;
		CRect r;
		m_cTab1.GetClientRect (r);
		m_cTab1.AdjustRect (FALSE, r);
		m_pCurrentPage->MoveWindow(r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
	}
}


LONG CuDlgReplicationStaticPageActivity::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int i, j, nInputQueue, nDistributionQueue;
	try
	{
		LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
		switch (pUps->nIpmHint)
		{
		case 0:
			break;
		default:
			return 0L;
		}
		
		ASSERT (pUps);
		LPRESOURCEDATAMIN pSvrDta = (LPRESOURCEDATAMIN)pUps->pStruct;

		CdIpmDoc* pIpmDoc = (CdIpmDoc*)wParam;
		CaIpmQueryInfo queryInfo (pIpmDoc, OT_DATABASE);
		BOOL bOK = IPM_ReplicationGetQueues(pIpmDoc, pSvrDta, nInputQueue, nDistributionQueue);
		if (bOK)
		{
			m_strInputQueue.Format (_T("%d"), nInputQueue);
			m_strDistributionQueue.Format (_T("%d"), nDistributionQueue);
		}
		else
		{
			m_strInputQueue         = _T("???");
			m_strDistributionQueue  = _T("???");
		}
		// the queue counts have been refreshed. But the dbevent counts should not
		// be reset upon a background / or force refresh (they are refreshed (increased)
		// only when they get notified that a DBEvent had been received). 
		if (!pSvrDta->m_bRefresh) {
			UpdateData (FALSE);
			return 0L;
		}
		pSvrDta->m_bRefresh = FALSE;
		
		CTime t = CTime::GetCurrentTime();
		m_strStartingTime   = t.Format (_T("%c"));
		UpdateData (FALSE);
		for (i=0; i<3; i++)
			for (j=1; j<6; j++)
				m_cListCtrl.SetItemText (i, j, _T("0"));
		m_pDatabaseIncoming->m_cListCtrl.DeleteAllItems();
		m_pDatabaseOutgoing->m_cListCtrl.DeleteAllItems();
		m_pDatabaseTotal->m_cListCtrl.DeleteAllItems();
		m_pTableIncoming->m_cListCtrl.DeleteAllItems();
		m_pTableOutgoing->m_cListCtrl.DeleteAllItems();
		m_pTableTotal->m_cListCtrl.DeleteAllItems();
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch(...)
	{
	}
	return 0L;
}

LONG CuDlgReplicationStaticPageActivity::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationStaticDataPageActivity")) == 0);
	CaReplicationStaticDataPageActivity* pData = (CaReplicationStaticDataPageActivity*)lParam;
	if (!pData)
		return 0L;
	POSITION p, pos = NULL;
	CStringList* pObj = NULL;
	try
	{
		m_strInputQueue        = pData->m_strInputQueue;
		m_strDistributionQueue = pData->m_strDistributionQueue;
		m_strStartingTime      = pData->m_strStartingTime;
		UpdateData (FALSE);
		//
		// Summary:
		//
		// For each column:
		const int LAYOUT_NUMBER = 6;
		for (int i=0; i<LAYOUT_NUMBER; i++)
			m_cListCtrl.SetColumnWidth(i, pData->m_cxSummary.GetAt(i));
		int nCount = 0;
		pos = pData->m_listSummary.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pData->m_listSummary.GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 6);
			nCount = m_cListCtrl.GetItemCount();
			p = pObj->GetHeadPosition();
			m_cListCtrl.InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 5, (LPCTSTR)pObj->GetNext(p));
		}
		m_cListCtrl.SetScrollPos (SB_HORZ, pData->m_scrollSummary.cx);
		m_cListCtrl.SetScrollPos (SB_VERT, pData->m_scrollSummary.cy);
		CuListCtrl* pList = NULL;

		//
		// Per database (Outgoing):
		m_pDatabaseOutgoing->SetAllColumnWidth (pData->m_cxDatabaseOutgoing);
		pList = &(m_pDatabaseOutgoing->m_cListCtrl);
		pos = pData->m_listDatabaseOutgoing.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pData->m_listDatabaseOutgoing.GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 6);
			nCount = pList->GetItemCount();
			p = pObj->GetHeadPosition();
			pList->InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 5, (LPCTSTR)pObj->GetNext(p));
		}
		m_pDatabaseOutgoing->SetScrollPosListCtrl (pData->m_scrollDatabaseOutgoing);

		//
		// Per database (Incoming):
		m_pDatabaseIncoming->SetAllColumnWidth (pData->m_cxDatabaseIncoming);
		pList = &(m_pDatabaseIncoming->m_cListCtrl);
		pos = pData->m_listDatabaseIncoming.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pData->m_listDatabaseIncoming.GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 6);
			nCount = pList->GetItemCount();
			p = pObj->GetHeadPosition();
			pList->InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 5, (LPCTSTR)pObj->GetNext(p));
		}
		m_pDatabaseIncoming->SetScrollPosListCtrl (pData->m_scrollDatabaseIncoming);
		//
		// Per database (Total):
		m_pDatabaseTotal->SetAllColumnWidth (pData->m_cxDatabaseTotal);
		pList = &(m_pDatabaseTotal->m_cListCtrl);
		pos = pData->m_listDatabaseTotal.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pData->m_listDatabaseTotal.GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 6);
			nCount = pList->GetItemCount();
			p = pObj->GetHeadPosition();
			pList->InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 5, (LPCTSTR)pObj->GetNext(p));
		}
		m_pDatabaseTotal->SetScrollPosListCtrl (pData->m_scrollDatabaseTotal);

		//
		// Per table (Outgoing):
		m_pTableOutgoing->SetAllColumnWidth (pData->m_cxTableOutgoing);
		pList = &(m_pTableOutgoing->m_cListCtrl);
		pos = pData->m_listTableOutgoing.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pData->m_listTableOutgoing.GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 6);
			nCount = pList->GetItemCount();
			p = pObj->GetHeadPosition();
			pList->InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 5, (LPCTSTR)pObj->GetNext(p));
		}
		m_pTableOutgoing->SetScrollPosListCtrl (pData->m_scrollTableOutgoing);
		//
		// Per table (Incoming):
		m_pTableIncoming->SetAllColumnWidth (pData->m_cxTableIncoming);
		pList = &(m_pTableIncoming->m_cListCtrl);
		pos = pData->m_listTableIncoming.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pData->m_listTableIncoming.GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 6);
			nCount = pList->GetItemCount();
			p = pObj->GetHeadPosition();
			pList->InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 5, (LPCTSTR)pObj->GetNext(p));
		}
		m_pTableIncoming->SetScrollPosListCtrl (pData->m_scrollTableIncoming);
		//
		// Per table (Total):
		m_pTableTotal->SetAllColumnWidth (pData->m_cxTableTotal);
		pList = &(m_pTableTotal->m_cListCtrl);
		pos = pData->m_listTableTotal.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pData->m_listTableTotal.GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 6);
			nCount = pList->GetItemCount();
			p = pObj->GetHeadPosition();
			pList->InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
			pList->SetItemText (nCount, 5, (LPCTSTR)pObj->GetNext(p));
		}	
		m_pTableTotal->SetScrollPosListCtrl (pData->m_scrollTableTotal);
		DisplayPage (pData->m_nCurrentPage);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgReplicationStaticPageActivity::OnGetData (WPARAM wParam, LPARAM lParam)
{
	int i, nCount;
	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_WIDTH;

	CaReplicationStaticDataPageActivity* pData = NULL;
	CString strItem;
	CString strDatabase;
	CString strTable;
	CString strInsert;
	CString strUpdate;
	CString strDelete;
	CString strTotal;
	CString strTransaction;

	try
	{
		pData = new CaReplicationStaticDataPageActivity();
		pData->m_strInputQueue        = m_strInputQueue;
		pData->m_strDistributionQueue = m_strDistributionQueue;
		pData->m_strStartingTime      = m_strStartingTime;

		//
		// Summary:
		nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CStringList* pObj = new CStringList();
			strItem        = m_cListCtrl.GetItemText (i, 0);
			strInsert      = m_cListCtrl.GetItemText (i, 1);
			strUpdate      = m_cListCtrl.GetItemText (i, 2);
			strDelete      = m_cListCtrl.GetItemText (i, 3);
			strTotal       = m_cListCtrl.GetItemText (i, 4);
			strTransaction = m_cListCtrl.GetItemText (i, 5);
			pObj->AddTail (strItem);
			pObj->AddTail (strInsert);
			pObj->AddTail (strUpdate);
			pObj->AddTail (strDelete);
			pObj->AddTail (strTotal);
			pObj->AddTail (strTransaction);
			pData->m_listSummary.AddTail (pObj);
		}
		//
		// For each column
		const int LAYOUT_NUMBER = 6;
		int hWidth   [LAYOUT_NUMBER] = {120, 60, 60, 60, 60, 100};
		for (i=0; i<LAYOUT_NUMBER; i++)
		{
			if (m_cListCtrl.GetColumn(i, &lvcolumn))
				pData->m_cxSummary.InsertAt (i, lvcolumn.cx);
			else
				pData->m_cxSummary.InsertAt (i, hWidth[i]);
		}
		pData->m_scrollSummary = CSize (m_cListCtrl.GetScrollPos (SB_HORZ), m_cListCtrl.GetScrollPos (SB_VERT));

		CuListCtrl* pList = NULL;
		//
		// Per database (Outgoing):
		m_pDatabaseOutgoing->GetAllColumnWidth (pData->m_cxDatabaseOutgoing);
		pData->m_scrollDatabaseOutgoing = m_pDatabaseOutgoing->GetScrollPosListCtrl();
		pList = &(m_pDatabaseOutgoing->m_cListCtrl);
		nCount = pList->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CStringList* pObj = new CStringList();
			strDatabase    = m_cListCtrl.GetItemText (i, 0);
			strInsert      = m_cListCtrl.GetItemText (i, 1);
			strUpdate      = m_cListCtrl.GetItemText (i, 2);
			strDelete      = m_cListCtrl.GetItemText (i, 3);
			strTotal       = m_cListCtrl.GetItemText (i, 4);
			strTransaction = m_cListCtrl.GetItemText (i, 5);
			pObj->AddTail (strDatabase);
			pObj->AddTail (strInsert);
			pObj->AddTail (strUpdate);
			pObj->AddTail (strDelete);
			pObj->AddTail (strTotal);
			pObj->AddTail (strTransaction);
			pData->m_listDatabaseOutgoing.AddTail (pObj);
		}
		//
		// Per database (Incoming):
		m_pDatabaseIncoming->GetAllColumnWidth (pData->m_cxDatabaseIncoming);
		pData->m_scrollDatabaseIncoming = m_pDatabaseIncoming->GetScrollPosListCtrl();
		pList = &(m_pDatabaseIncoming->m_cListCtrl);
		nCount = pList->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CStringList* pObj = new CStringList();
			strDatabase    = m_cListCtrl.GetItemText (i, 0);
			strInsert      = m_cListCtrl.GetItemText (i, 1);
			strUpdate      = m_cListCtrl.GetItemText (i, 2);
			strDelete      = m_cListCtrl.GetItemText (i, 3);
			strTotal       = m_cListCtrl.GetItemText (i, 4);
			strTransaction = m_cListCtrl.GetItemText (i, 5);
			pObj->AddTail (strDatabase);
			pObj->AddTail (strInsert);
			pObj->AddTail (strUpdate);
			pObj->AddTail (strDelete);
			pObj->AddTail (strTotal);
			pObj->AddTail (strTransaction);
			pData->m_listDatabaseIncoming.AddTail (pObj);
		}
		//
		// Per database (Total):
		m_pDatabaseTotal->GetAllColumnWidth (pData->m_cxDatabaseTotal);
		pData->m_scrollDatabaseTotal = m_pDatabaseTotal->GetScrollPosListCtrl();
		pList = &(m_pDatabaseTotal->m_cListCtrl);
		nCount = pList->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CStringList* pObj = new CStringList();
			strDatabase    = m_cListCtrl.GetItemText (i, 0);
			strInsert      = m_cListCtrl.GetItemText (i, 1);
			strUpdate      = m_cListCtrl.GetItemText (i, 2);
			strDelete      = m_cListCtrl.GetItemText (i, 3);
			strTotal       = m_cListCtrl.GetItemText (i, 4);
			strTransaction = m_cListCtrl.GetItemText (i, 5);
			pObj->AddTail (strDatabase);
			pObj->AddTail (strInsert);
			pObj->AddTail (strUpdate);
			pObj->AddTail (strDelete);
			pObj->AddTail (strTotal);
			pObj->AddTail (strTransaction);
			pData->m_listDatabaseTotal.AddTail (pObj);
		}

		//
		// Per table (Outgoing):
		m_pTableOutgoing->GetAllColumnWidth (pData->m_cxTableOutgoing);
		pData->m_scrollTableOutgoing = m_pTableOutgoing->GetScrollPosListCtrl();
		pList = &(m_pTableOutgoing->m_cListCtrl);
		nCount = pList->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CStringList* pObj = new CStringList();
			strDatabase    = m_cListCtrl.GetItemText (i, 0);
			strTable       = m_cListCtrl.GetItemText (i, 1);
			strInsert      = m_cListCtrl.GetItemText (i, 2);
			strUpdate      = m_cListCtrl.GetItemText (i, 3);
			strDelete      = m_cListCtrl.GetItemText (i, 4);
			strTotal       = m_cListCtrl.GetItemText (i, 5);
			pObj->AddTail (strDatabase);
			pObj->AddTail (strTable);
			pObj->AddTail (strInsert);
			pObj->AddTail (strUpdate);
			pObj->AddTail (strDelete);
			pObj->AddTail (strTotal);
			pData->m_listTableOutgoing.AddTail (pObj);
		}
		//
		// Per table (Incoming):
		m_pTableIncoming->GetAllColumnWidth (pData->m_cxTableIncoming);
		pData->m_scrollTableIncoming = m_pTableIncoming->GetScrollPosListCtrl();
		pList = &(m_pTableIncoming->m_cListCtrl);
		nCount = pList->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CStringList* pObj = new CStringList();
			strDatabase    = m_cListCtrl.GetItemText (i, 0);
			strTable       = m_cListCtrl.GetItemText (i, 1);
			strInsert      = m_cListCtrl.GetItemText (i, 2);
			strUpdate      = m_cListCtrl.GetItemText (i, 3);
			strDelete      = m_cListCtrl.GetItemText (i, 4);
			strTotal       = m_cListCtrl.GetItemText (i, 5);
			pObj->AddTail (strDatabase);
			pObj->AddTail (strTable);
			pObj->AddTail (strInsert);
			pObj->AddTail (strUpdate);
			pObj->AddTail (strDelete);
			pObj->AddTail (strTotal);
			pData->m_listTableIncoming.AddTail (pObj);
		}
		//
		// Per table (Tatal):
		m_pTableTotal->GetAllColumnWidth (pData->m_cxTableTotal);
		pData->m_scrollTableTotal = m_pTableTotal->GetScrollPosListCtrl();
		pList = &(m_pTableTotal->m_cListCtrl);
		nCount = pList->GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CStringList* pObj = new CStringList();
			strDatabase    = m_cListCtrl.GetItemText (i, 0);
			strTable       = m_cListCtrl.GetItemText (i, 1);
			strInsert      = m_cListCtrl.GetItemText (i, 2);
			strUpdate      = m_cListCtrl.GetItemText (i, 3);
			strDelete      = m_cListCtrl.GetItemText (i, 4);
			strTotal       = m_cListCtrl.GetItemText (i, 5);
			pObj->AddTail (strDatabase);
			pObj->AddTail (strTable);
			pObj->AddTail (strInsert);
			pObj->AddTail (strUpdate);
			pObj->AddTail (strDelete);
			pObj->AddTail (strTotal);
			pData->m_listTableTotal.AddTail (pObj);
		}
		pData->m_nCurrentPage = m_cTab1.GetCurSel();
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

LONG CuDlgReplicationStaticPageActivity::OnLeavingPage(WPARAM wParam, LPARAM lParam)
{
	return 0L;
}


long CuDlgReplicationStaticPageActivity::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	if (m_pDatabaseIncoming)
		PropertyChange(&(m_pDatabaseIncoming->m_cListCtrl), wParam, lParam);
	if (m_pDatabaseOutgoing)
		PropertyChange(&(m_pDatabaseOutgoing->m_cListCtrl), wParam, lParam);
	if (m_pDatabaseTotal)
		PropertyChange(&(m_pDatabaseTotal->m_cListCtrl), wParam, lParam);
	if (m_pTableIncoming)
		PropertyChange(&(m_pTableIncoming->m_cListCtrl), wParam, lParam);
	if (m_pTableOutgoing)
		PropertyChange(&(m_pTableOutgoing->m_cListCtrl), wParam, lParam);
	if (m_pTableTotal)
		PropertyChange(&(m_pTableTotal->m_cListCtrl), wParam, lParam);
	return 0;
}
