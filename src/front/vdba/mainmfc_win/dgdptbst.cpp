/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdptbst.cpp, Implementation file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Table's statistic pane of DOM's right pane.
** 
** History after 02-May-2001:
** xx-Mar-1998 (uk$so01):
**    Created.
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 08-Oct-2001 (schph01)
**    SIR #105881
**    Generate an error message when trying to generate statistics on an
**    Index with only one column in the key.
*/


#include "stdafx.h"
#include "mainmfc.h"
#include "dgdptbst.h"
#include "parse.h"
#include "rcdepend.h"
#include "vtree.h"
#include "sqlselec.h"

extern "C"
{
#include "dba.h"
#include "dbaset.h"
#include "main.h"
#include "dbaginfo.h"
#include "dom.h"
#include "domdata.h"
#include "resource.h"
#include "tree.h"
#include "msghandl.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableStatistic dialog


CuDlgDomPropTableStatistic::CuDlgDomPropTableStatistic(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropTableStatistic::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropTableStatistic)
	m_bUniqueFlag = FALSE;
	m_bCompleteFlag = FALSE;
	m_strUniqueValues = _T("");
	m_strRepetitionFactors = _T("");
	//}}AFX_DATA_INIT
	m_bLoad = FALSE;
	m_bCleaned = FALSE;
	m_bExecuted=FALSE;
	m_nOT = -1;
}

void CuDlgDomPropTableStatistic::CleanStatisticItem()
{
	CaTableStatisticItem* pItemStat = NULL;
	int i, nCount = m_cListStatItem.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pItemStat = (CaTableStatisticItem*)m_cListStatItem.GetItemData (i);
		if (pItemStat)
			delete pItemStat;
	}
	m_statisticData.Cleanup();
	m_cListStatItem.DeleteAllItems();
}

void CuDlgDomPropTableStatistic::CleanListColumns()
{
	m_statisticData.Cleanup();
	CaTableStatisticColumn* pColumn = NULL;
	int i, nCount = m_cListColumn.GetItemCount();

	for (i=0; i<nCount; i++)
	{
		pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData (i);
		if (pColumn)
			delete pColumn;
	}

	m_cListColumn.DeleteAllItems();
}


void CuDlgDomPropTableStatistic::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropTableStatistic)
	DDX_Control(pDX, IDC_MFC_BUTTON2, m_cButtonRemove);
	DDX_Control(pDX, IDC_MFC_BUTTON1, m_cButtonGenerate);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bUniqueFlag);
	DDX_Check(pDX, IDC_MFC_CHECK2, m_bCompleteFlag);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strUniqueValues);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strRepetitionFactors);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTableStatistic, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropTableStatistic)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MFC_LIST1, OnItemchangedList1)
	ON_NOTIFY(NM_OUTOFMEMORY, IDC_MFC_LIST1, OnOutofmemoryList1)
	ON_NOTIFY(NM_OUTOFMEMORY, IDC_MFC_LIST2, OnOutofmemoryList2)
	ON_BN_CLICKED(IDC_MFC_BUTTON1, OnGenerate)
	ON_BN_CLICKED(IDC_MFC_BUTTON2, OnRemove)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_MFC_LIST1, OnColumnclickList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING,   OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,     OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,     OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE,   OnQueryType)
	ON_MESSAGE (WM_QUERY_UPDATING,           OnUpdateQuery)
	ON_MESSAGE (WM_USER_IPMPAGE_LEAVINGPAGE, OnChangeProperty)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableStatistic message handlers

LONG CuDlgDomPropTableStatistic::OnUpdateQuery (WPARAM wParam, LPARAM lParam)
{
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (pUps);
	if (pUps == NULL)
		return 0L;
	pUps->nIpmHint = -1;
	return 0L;
}

LONG CuDlgDomPropTableStatistic::OnChangeProperty (WPARAM wParam, LPARAM lParam)
{
	if ((int)lParam == 0)
	{
		//
		// Left pane selection changes:
		m_bExecuted = FALSE;
	}
	else
	if ((int)lParam == 1)
	{
		//
		// Right pane Tab selection changes:
	}
	return 0L;
}

void CuDlgDomPropTableStatistic::PostNcDestroy() 
{
	ASSERT (m_bCleaned); // ON_WM_DESTROY is not called !!!
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgDomPropTableStatistic::OnDestroy() 
{
	CleanStatisticItem();
	CleanListColumns();
	m_statisticData.Cleanup();
	m_bCleaned = TRUE;
	CDialog::OnDestroy();
}

BOOL CuDlgDomPropTableStatistic::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListColumn.SubclassDlgItem (IDC_MFC_LIST1, this));
	LONG style = GetWindowLong (m_cListColumn.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_cListColumn.m_hWnd, GWL_STYLE, style);
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListColumn.SetCheckImageList(&m_ImageCheck);

	m_ImageList.Create(1, 23, TRUE, 1, 0);
	VERIFY (m_cListStatItem.SubclassDlgItem (IDC_MFC_LIST2, this));
	m_cListStatItem.SetImageList (&m_ImageList, LVSIL_SMALL);

	int i;
	LV_COLUMN lvcolumn;
	const int LAYOUT_NUMBER_STATITEM = 6;
	LSCTRLHEADERPARAMS statHeader [LAYOUT_NUMBER_STATITEM] = 
	{
		{_T("#"),     30, LVCFMT_RIGHT, FALSE},
		{_T(""),      80, LVCFMT_LEFT,  FALSE},
		{_T(""),      50, LVCFMT_RIGHT, FALSE},
		{_T(""),      100, LVCFMT_LEFT,  FALSE},
		{_T(""),      60, LVCFMT_RIGHT, FALSE},
		{_T(""),      100, LVCFMT_LEFT,  FALSE}
	};
	lstrcpy(statHeader [1].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_VALUE));      //_T("Value")
	lstrcpy(statHeader [2].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_COUNT));      //_T("Count")
	lstrcpy(statHeader [3].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_GRAPH));      //_T("Graph")
	lstrcpy(statHeader [4].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_REP_FACTOR)); //_T("Rep Factor")
	lstrcpy(statHeader [5].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_GRAPH));      //_T("Graph")

	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER_STATITEM; i++)
	{
		lvcolumn.fmt = statHeader[i].m_fmt;
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)statHeader[i].m_tchszHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = statHeader[i].m_cxWidth;
		m_cListStatItem.InsertColumn (i, &lvcolumn, statHeader[i].m_bUseCheckMark);
	}

	EnableButtons();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropTableStatistic::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListColumn.m_hWnd))
		return;
	CRect rc, rcDlg;
	GetClientRect (rcDlg);
	
	m_cListColumn.GetWindowRect (rc);
	ScreenToClient (rc);
	if (rcDlg.right > rc.left)
	{
		//
		// Attachement to the right of its parent:
		rc.right = rcDlg.right;
		m_cListColumn.MoveWindow (rc);
	}
	
	BOOL bSize = FALSE;
	m_cListStatItem.GetWindowRect (rc);
	ScreenToClient (rc);
	if (rcDlg.right > rc.left)
	{
		//
		// Attachement to the right of its parent:
		rc.right = rcDlg.right;
		bSize   = TRUE;
	}
	if (rcDlg.bottom > rc.top)
	{
		//
		// Attachement to the bottom of its parent:
		rc.bottom = rcDlg.bottom;
		bSize   = TRUE;
	}
	if (bSize)
		m_cListStatItem.MoveWindow (rc);
}

void CuDlgDomPropTableStatistic::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	if (m_bLoad)
		return;

	CaTableStatisticColumn* pColumn = NULL;
	if (pNMListView->iItem >= 0 && pNMListView->uNewState > 0 && (pNMListView->uNewState&LVIS_SELECTED))
	{
		CWaitCursor doWaitCursor;
		try
		{
			pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData(pNMListView->iItem);
			CleanStatisticItem();
			if (!pColumn)
				return;
			if (!pColumn->m_bHasStatistic)
				return;
			//
			// Query the statistic for this column:
			//
			Table_GetStatistics (pColumn, m_statisticData);
			// Extra information:
			m_bUniqueFlag          = m_statisticData.m_bUniqueFlag;
			m_bCompleteFlag        = m_statisticData.m_bCompleteFlag;
			m_strUniqueValues.Format (_T("%ld"), m_statisticData.m_lUniqueValue);
			m_strRepetitionFactors.Format (_T("%ld"), m_statisticData.m_lRepetitionFlag);
			DrawStatistic ();
		}
		catch (CMemoryException* e)
		{
			VDBA_OutOfMemoryMessage();
			e->Delete();
		}
		catch (...)
		{
			//CString strMsg = _T("Internal error: cannot display the statistic of the table.");
			AfxMessageBox (VDBA_MfcResourceString(IDS_E_STAT_TABLE));
		}
	}
	UpdateData (FALSE);
	EnableButtons();

}

void CuDlgDomPropTableStatistic::OnOutofmemoryList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	VDBA_OutOfMemoryMessage();
	*pResult = 1;
}

void CuDlgDomPropTableStatistic::OnOutofmemoryList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	VDBA_OutOfMemoryMessage();
	*pResult = 1;
}


LONG CuDlgDomPropTableStatistic::OnQueryType(WPARAM wParam, LPARAM lParam)
{
	ASSERT (wParam == 0);
	ASSERT (lParam == 0);
	return DOMPAGE_SPECIAL;
}

LONG CuDlgDomPropTableStatistic::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	// cast received parameters
	int nNodeHandle = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (nNodeHandle != -1);
	ASSERT (pUps);
	if (pUps->nIpmHint == FILTER_DOM_BKREFRESH) {
		ASSERT (FALSE);   // Should not occur!
		return 0L;
	}

	try
	{
		if (pUps->nType != m_nOT)
		{
			CleanStatisticItem();
			CleanListColumns();
			InitializeStatisticHeader(pUps->nType);
		}
		m_nOT = pUps->nType;

		int nNodeHandle = (int)wParam;
		BOOL bCheck = FALSE; 
		LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
		ASSERT (nNodeHandle != -1);
		ASSERT (pUps);
		LPTREERECORD  lpRecord = (LPTREERECORD)pUps->pStruct;
		ASSERT (lpRecord);
		if (!lpRecord)
			return 0L;
		m_statisticData.Cleanup();
		//
		// Ignore selected actions on filters
		switch (pUps->nIpmHint)
		{
		case 0:
			//
			// Normal updating
			break;
		case -1:
			//
			// Selection changes in the Right pane 
			bCheck = TRUE;
			break;
		default:
			//
			// Nothing to change on the display
			return 0L;
		}
		
		//
		// Check if we need the update or not:
		if (bCheck && m_bExecuted)
		{
			if (m_statisticData.m_strDBName.CompareNoCase ((const char *)lpRecord->extra) == 0)
				if (m_statisticData.m_strTable.CompareNoCase ((const char *)lpRecord->objName) == 0)
					if (m_statisticData.m_strTableOwner.CompareNoCase ((const char *)lpRecord->ownerName) == 0)
						return 0L;
		}

		m_statisticData.m_nOT = m_nOT;
		m_statisticData.m_strVNode      = (LPCTSTR)(LPUCHAR)GetVirtNodeName (nNodeHandle);
		m_statisticData.m_strDBName     = (const char *)lpRecord->extra;
		if (m_nOT == OT_TABLE)
		{
			m_statisticData.m_strTable      = RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName));
			m_statisticData.m_strTableOwner = (const char *)lpRecord->ownerName;
		}
		else
		if (m_nOT == OT_INDEX)
		{
			m_statisticData.m_strTable      = lpRecord->extra2;
			m_statisticData.m_strTableOwner = (const char *)lpRecord->ownerName;
			m_statisticData.m_strIndex      = RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(lpRecord->objName));
		}

		CleanStatisticItem();
		QueryStatColumns();
		
		if (m_cListColumn.GetItemCount() > 0)
		{
			//
			// This line cause to execute the member: OnItemchangedList1()
			m_cListColumn.SetItemState (0, LVIS_SELECTED, LVIS_SELECTED);
		}
		UpdateData (FALSE);
		m_bExecuted = TRUE;
		EnableButtons();
		return 0L;
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeSQLException e)
	{
		AfxMessageBox (e.m_strReason, MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
		//CString strMsg = _T("Internal error: cannot display the statistic of the table.");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_STAT_TABLE));
	}
	CleanStatisticItem();
	m_statisticData.Cleanup();
	return 0L;
}

LONG CuDlgDomPropTableStatistic::OnLoad (WPARAM wParam, LPARAM lParam)
{
	int nCount = 0;
	try
	{
		LPCTSTR pClass = (LPCTSTR)wParam;
		ASSERT (lstrcmp (pClass, "CuDomPropDataTableStatistic") == 0);
		CuDomPropDataTableStatistic* pData = (CuDomPropDataTableStatistic*)lParam;
		m_nOT = pData->m_statisticData.m_nOT;
		m_statisticData = pData->m_statisticData;
		InitializeStatisticHeader(m_statisticData.m_nOT);

		//
		// List of statistic columns;
		CaTableStatisticColumn* pColumn = NULL;
		while (!m_statisticData.m_listColumn.IsEmpty())
		{
			pColumn = m_statisticData.m_listColumn.RemoveHead();
			if (m_cListColumn.InsertItem (nCount, "") != -1)
			{
				m_cListColumn.SetItemData (nCount, (DWORD)pColumn);
				nCount++;
			}
		}
		UpdateDisplayList1();
		m_bLoad = TRUE; // Prevent from calling OnItemchangedList1()
		m_cListColumn.SetItemState (pData->m_nSelectColumn, LVIS_SELECTED, LVIS_SELECTED);
		m_bLoad = FALSE;
		//
		// Draw statistic Items:
		DrawStatistic ();
		//
		// Extra information:
		m_bUniqueFlag          = m_statisticData.m_bUniqueFlag;
		m_bCompleteFlag        = m_statisticData.m_bCompleteFlag;
		m_strUniqueValues.Format (_T("%ld"), m_statisticData.m_lUniqueValue);
		m_strRepetitionFactors.Format (_T("%ld"), m_statisticData.m_lRepetitionFlag);
		UpdateData (FALSE);
		m_bExecuted = TRUE;
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		//CString strMsg = _T("Internal error: cannot display the statistic of the table.");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_STAT_TABLE));
	}
	EnableButtons();
	return 0L;
}

LONG CuDlgDomPropTableStatistic::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuDomPropDataTableStatistic* pData = NULL;
	CaTableStatisticColumn* pColumn = NULL;
	CaTableStatisticColumn* pNewColumn = NULL;
	CaTableStatisticItem* pStatItem = NULL;
	CaTableStatisticItem* pNewStatItem = NULL;
	int i, nCount;
	try
	{
		nCount = m_cListColumn.GetItemCount();
		pData  = new CuDomPropDataTableStatistic();
		pData->m_statisticData = m_statisticData;
		pData->m_nSelectColumn = -1;
		for (i=0; i<nCount; i++)
		{
			pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData(i);
			pNewColumn = new CaTableStatisticColumn();
			*pNewColumn= *pColumn;
			pData->m_statisticData.m_listColumn.AddTail(pNewColumn);
			if (m_cListColumn.GetItemState (i, LVIS_SELECTED) & LVIS_SELECTED)
				pData->m_nSelectColumn = i;
		}

		nCount = m_cListStatItem.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			pStatItem = (CaTableStatisticItem*)m_cListStatItem.GetItemData(i);
			if (!pStatItem)
				continue;
			pNewStatItem = new CaTableStatisticItem();
			*pNewStatItem= *pStatItem;
			pData->m_statisticData.m_listItem.AddTail (pNewStatItem);
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		//CString strMsg = _T("Cannot allocate data for storing.");
		AfxMessageBox (VDBA_MfcResourceString(IDS_E_ALLOCATE_DATA));
		pData = NULL;
	}
	return (LRESULT)pData;
}

BOOL CuDlgDomPropTableStatistic::QueryStatColumns()
{
	int nCount = 0;
	CaTableStatisticColumn* pColumn = NULL;
	CleanListColumns();
	int nIndex = 0;
	if (!Table_QueryStatisticColumns (m_statisticData))
		return FALSE;

	while (!m_statisticData.m_listColumn.IsEmpty())
	{
		pColumn = m_statisticData.m_listColumn.RemoveHead();
		if (m_cListColumn.InsertItem (nCount, "") != -1)
		{
			m_cListColumn.SetItemData (nCount, (DWORD)pColumn);
			nCount++;
		}
	}
	UpdateDisplayList1();
	
	//
	// Sort the default:
	SORTPARAMS s;
	m_statisticData.m_bAsc         = FALSE;
	m_statisticData.m_nCurrentSort = CaTableStatisticColumn::COLUMN_STATISTIC;
	s.m_bAsc  = m_statisticData.m_bAsc;
	s.m_nItem = m_statisticData.m_nCurrentSort;
	m_cListColumn.SortItems(CaTableStatisticColumn::Compare, (LPARAM)&s);
	return TRUE;
}

void CuDlgDomPropTableStatistic::OnGenerate() 
{
	CWaitCursor doWaitCursor;
	SORTPARAMS s;
	s.m_bAsc  = m_statisticData.m_bAsc;
	s.m_nItem = m_statisticData.m_nCurrentSort;
	int nSelected = m_cListColumn.GetNextItem (-1, LVNI_SELECTED);

	if (nSelected != -1)
	{
		CaTableStatisticColumn* pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData (nSelected);
		if (!pColumn)
			return;
		if(pColumn->m_bComposite && m_statisticData.m_nNumColumnKey<2)
		{
			if (m_statisticData.m_nOT == OT_TABLE)
			{
				//
				// There is only one column in the key structure of the base table,
				// therefore a composite histogram cannot be generated.
				//
				AfxMessageBox(VDBA_MfcResourceString(IDS_ERR_TBL_ONLY_WITH_TWO_KEY_COL));
				return;
			}
			else if (m_statisticData.m_nOT == OT_INDEX)
			{
				//
				// Statistics cannot be generated on this index since it has only
				// 1 key column. This operation is used to generate a composite histogram.
				AfxMessageBox(VDBA_MfcResourceString(IDS_ERR_ONLY_WITH_TWO_KEY_COL));
				return;
			}
		}

		CString strColumn = pColumn->m_strColumn;

		if (!Table_GenerateStatistics (this->m_hWnd ,pColumn, m_statisticData))
			return;
		if (!QueryStatColumns())
			return;
		m_cListColumn.SortItems(CaTableStatisticColumn::Compare, (LPARAM)&s);
		//
		// Try to selected the old selected item:
		nSelected = Find (strColumn);
		if (nSelected != -1)
		{
			//
			// This line cause to execute the member: OnItemchangedList1()
			// and then draw the statistic:
			m_cListColumn.SetItemState (nSelected, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		}
	}
	EnableButtons();
}

void CuDlgDomPropTableStatistic::OnRemove() 
{
	CWaitCursor doWaitCursor;
	SORTPARAMS s;
	s.m_bAsc  = m_statisticData.m_bAsc;
	s.m_nItem = m_statisticData.m_nCurrentSort;
	int nSelected = m_cListColumn.GetNextItem (-1, LVNI_SELECTED);

	if (nSelected != -1)
	{
		CaTableStatisticColumn* pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData (nSelected);
		if (!pColumn)
			return;
		CString strColumn = pColumn->m_strColumn;

		if (!Table_RemoveStatistics (pColumn, m_statisticData))
			return;
		if (!QueryStatColumns())
			return;
		m_cListColumn.SortItems(CaTableStatisticColumn::Compare, (LPARAM)&s);
		//
		// Try to selected the old selected item:
		nSelected = Find (strColumn);
		if (nSelected != -1)
		{
			//
			// This line cause to execute the member: OnItemchangedList1()
			// and then draw the statistic:
			m_cListColumn.SetItemState (nSelected, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		}
	}
	EnableButtons();
}



void CuDlgDomPropTableStatistic::DrawStatistic ()
{
	//
	// Set the max values for drawing the statistic:
	double maxPercent;
	double maxRepFactor;
	m_statisticData.MaxValues (maxPercent, maxRepFactor);
	m_cListStatItem.SetMaxValues (maxPercent, maxRepFactor);
	//
	// Draw the statistic:
	int nCount = 0;
	CaTableStatisticItem* pItemStatic = NULL;
	while (!m_statisticData.m_listItem.IsEmpty())
	{
		pItemStatic = m_statisticData.m_listItem.RemoveHead();

		m_cListStatItem.InsertItem  (nCount, _T(""));
		m_cListStatItem.SetItemData (nCount, (DWORD)pItemStatic);
		nCount++;
	}
}


void CuDlgDomPropTableStatistic::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SORTPARAMS s;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	m_statisticData.m_bAsc = (m_statisticData.m_nCurrentSort == pNMListView->iSubItem)? !m_statisticData.m_bAsc: TRUE;
	m_statisticData.m_nCurrentSort = pNMListView->iSubItem;
	s.m_bAsc  = m_statisticData.m_bAsc;
	s.m_nItem = m_statisticData.m_nCurrentSort;
	m_cListColumn.SortItems(CaTableStatisticColumn::Compare, (LPARAM)&s);
	*pResult = 0;
}



void CuDlgDomPropTableStatistic::UpdateDisplayList1()
{
	CString strType;
	CString strPKey;
	CaTableStatisticColumn* pColumn = NULL;
	int i, nCount = m_cListColumn.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData(i);
		if (!pColumn)
			continue;
		if (m_nOT == OT_TABLE)
		{
			strType = pColumn->m_strType;
			if (pColumn->m_nPKey > 0)
				strPKey.Format (_T("%d"), pColumn->m_nPKey);
			else
				strPKey = _T("");
			m_cListColumn.SetCheck   (i, CaTableStatisticColumn::COLUMN_STATISTIC,  pColumn->m_bHasStatistic);
			m_cListColumn.SetItemText(i, CaTableStatisticColumn::COLUMN_NAME,       pColumn->m_strColumn);
			m_cListColumn.SetItemText(i, CaTableStatisticColumn::COLUMN_TYPE,       strType);
			m_cListColumn.SetItemText(i, CaTableStatisticColumn::COLUMN_PKEY,       strPKey);
			m_cListColumn.SetCheck   (i, CaTableStatisticColumn::COLUMN_NULLABLE,   pColumn->m_bNullable);
			m_cListColumn.SetCheck   (i, CaTableStatisticColumn::COLUMN_WITHDEFAULT,pColumn->m_bWithDefault);
			m_cListColumn.SetItemText(i, CaTableStatisticColumn::COLUMN_DEFAULT,    pColumn->m_strDefault);

			m_cListColumn.EnableCheck(i, CaTableStatisticColumn::COLUMN_STATISTIC,  FALSE);
			m_cListColumn.EnableCheck(i, CaTableStatisticColumn::COLUMN_NULLABLE,   FALSE);
			m_cListColumn.EnableCheck(i, CaTableStatisticColumn::COLUMN_WITHDEFAULT,FALSE);
		}
		else
		if (m_nOT == OT_INDEX)
		{
			m_cListColumn.SetCheck   (i, CaTableStatisticColumn::COLUMN_STATISTIC,  pColumn->m_bHasStatistic);
			m_cListColumn.SetItemText(i, CaTableStatisticColumn::COLUMN_NAME,       pColumn->m_strColumn);
		}
	}
}

void CuDlgDomPropTableStatistic::EnableButtons()
{
	BOOL bRemoveEnable = FALSE;
	if (m_cListColumn.GetSelectedCount() > 0)
	{
		int nSelected = m_cListColumn.GetNextItem (-1, LVNI_SELECTED);
		if (nSelected != -1)
		{
			CaTableStatisticColumn* pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData (nSelected);
			if (pColumn	&& pColumn->m_bHasStatistic)
				bRemoveEnable = TRUE;
		}
	}
	m_cButtonRemove.EnableWindow (bRemoveEnable);
}

int CuDlgDomPropTableStatistic::Find (LPCTSTR lpszColumn)
{
	CaTableStatisticColumn* pColumn = NULL;
	int i, nSelected = -1, nCount = m_cListColumn.GetItemCount();
	if (nCount == 0 || lpszColumn == NULL)
		return -1;
	for (i=0; i<nCount; i++)
	{
		pColumn = (CaTableStatisticColumn*)m_cListColumn.GetItemData (i);
		if (pColumn && pColumn->m_strColumn.CompareNoCase (lpszColumn) == 0)
			return i;
	}
	return -1;
}

void CuDlgDomPropTableStatistic::ResetDisplay()
{
	//
	// Nothing to do!
}

void CuDlgDomPropTableStatistic::InitializeStatisticHeader(int nOT)
{
	int i;
	LVCOLUMN lvcolumn;
	while (m_cListColumn.DeleteColumn(0));

	if (nOT == OT_TABLE)
	{
		//
		// Initalize the Column Header of CListCtrl (CuListCtrl)
		const int LAYOUT_NUMBER_COLUMN = 7;
		LSCTRLHEADERPARAMS columnHeader [LAYOUT_NUMBER_COLUMN] =
		{
			{_T(""),                50, LVCFMT_CENTER,  TRUE },
			{_T(""),                80, LVCFMT_LEFT,    FALSE},
			{_T(""),                80, LVCFMT_LEFT,    FALSE},
			{_T(""),                80, LVCFMT_RIGHT,   FALSE},
			{_T("With Null"),       60, LVCFMT_CENTER,  TRUE },
			{_T("With Default"),    80, LVCFMT_CENTER,  TRUE },
			{_T(""),                80, LVCFMT_LEFT,    FALSE}
		};
		lstrcpy(columnHeader [0].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_STAT));    //_T("Stat")
		lstrcpy(columnHeader [1].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_COLUMN));  //_T("Column")
		lstrcpy(columnHeader [2].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_TYPE));    //_T("Type"
		lstrcpy(columnHeader [3].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_PRIM_KEY));//_T("Prim.Key#")
		lstrcpy(columnHeader [6].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_DEF_SPEC));//_T("Default Specification")

		//
		// Set the number of columns: LAYOUT_NUMBER_COLUMN and LAYOUT_NUMBER_STATITEM
		//
		memset (&lvcolumn, 0, sizeof (LVCOLUMN));
		lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
		for (i=0; i<LAYOUT_NUMBER_COLUMN; i++)
		{
			lvcolumn.fmt = columnHeader[i].m_fmt;
			lvcolumn.pszText = (LPTSTR)(LPCTSTR)columnHeader[i].m_tchszHeader;
			lvcolumn.iSubItem = i;
			lvcolumn.cx = columnHeader[i].m_cxWidth;
			m_cListColumn.InsertColumn (i, &lvcolumn, columnHeader[i].m_bUseCheckMark);
		}
	}
	else
	if (nOT == OT_INDEX)
	{
		CString strHeader;
		//
		// Initalize the Column Header of CListCtrl (CuListCtrl)
		const int LAYOUT_NUMBER_COLUMN = 2;
		tagLSCTRLHEADERPARAMS2 columnHeader [LAYOUT_NUMBER_COLUMN] =
		{
			{IDS_TC_STAT,                50, LVCFMT_CENTER,  TRUE },
			{IDS_TC_COLUMN,             300, LVCFMT_LEFT,    FALSE}
		};
		//
		// Set the number of columns: LAYOUT_NUMBER_COLUMN and LAYOUT_NUMBER_STATITEM
		//
		memset (&lvcolumn, 0, sizeof (LVCOLUMN));
		lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
		for (i=0; i<LAYOUT_NUMBER_COLUMN; i++)
		{
			strHeader.LoadString(columnHeader[i].m_nIDS);
			lvcolumn.fmt = columnHeader[i].m_fmt;
			lvcolumn.pszText = (LPTSTR)(LPCTSTR)strHeader;
			lvcolumn.iSubItem = i;
			lvcolumn.cx = columnHeader[i].m_cxWidth;
			m_cListColumn.InsertColumn (i, &lvcolumn, columnHeader[i].m_bUseCheckMark);
		}
	}
	else
	{
		ASSERT (FALSE);
	}
	EnableButtons();
}

