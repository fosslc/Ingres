/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : cachelst.cpp, Implementation file
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Special Owner Draw List Control
**               Modeless dialog - left pane of page (DBMS Cache)
**
**  History:
**	xx-Sep-1997: (uk$so01) 
**	    created
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	01-Nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
**	13-Jun-2003 (schph01)
**	    (bug 98561)The 2k page size cache can be turned off.
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
*/

#include "stdafx.h"
#include "vcbf.h"
#include "cachefrm.h"
#include "cachevi1.h"
#include "cachevi2.h"
#include "cachelst.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgCacheList dialog

static void CleanCacheControl(CuCacheCheckListCtrl& list)
{
	CACHELINEINFO* pData = NULL;
	int i, nCount = list.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pData = (CACHELINEINFO*)list.GetItemData (i);
		if (pData)
			delete pData;
	}
}

CuDlgCacheList::CuDlgCacheList(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgCacheList::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgCacheList)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgCacheList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgCacheList)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgCacheList, CDialog)
	//{{AFX_MSG_MAP(CuDlgCacheList)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgCacheList message handlers

void CuDlgCacheList::PostNcDestroy() 
{
	delete this;	
	CDialog::PostNcDestroy();
}

void CuDlgCacheList::OnDestroy() 
{
	CleanCacheControl (m_ListCtrl);
	CDialog::OnDestroy();
}

BOOL CuDlgCacheList::OnInitDialog() 
{
	CDialog::OnInitDialog();
	const int LAYOUT_NUMBER = 2;
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN | LVS_SHOWSELALWAYS;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ImageListCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ListCtrl.SetCheckImageList(&m_ImageListCheck);

	//
	// Initalize the Column Header of CListCtrl
	//
	LV_COLUMN lvcolumn;
	UINT      strHeaderID [LAYOUT_NUMBER]     = { IDS_COL_CACHE, IDS_COL_STATUS};
	int       i, hWidth   [LAYOUT_NUMBER]     = {140, 86};
	int       fmt         [LAYOUT_NUMBER]     = {LVCFMT_LEFT, LVCFMT_CENTER};
	//
	// Set the number of columns: LAYOUT_NUMBER
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER; i++)
	{
		lvcolumn.fmt = fmt [i];
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		if (i == (LAYOUT_NUMBER-1))
			m_ListCtrl.InsertColumn (i, &lvcolumn, TRUE); 
		else
			m_ListCtrl.InsertColumn (i, &lvcolumn); 
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgCacheList::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect rDlg;
	GetClientRect (rDlg);
	m_ListCtrl.MoveWindow (rDlg);
}

void CuDlgCacheList::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (pNMListView->iItem != -1 && pNMListView->uNewState != 0 && (pNMListView->uNewState & LVIS_SELECTED))
	{
		CACHELINEINFO* pData = (CACHELINEINFO*)m_ListCtrl.GetItemData (pNMListView->iItem);
		try
		{
			CWnd* pParent1 = GetParent ();              // View (CvDbmsCacheViewLeft)
			ASSERT_VALID (pParent1);
			CWnd* pParent2 = pParent1->GetParent ();    // SplitterWnd
			ASSERT_VALID (pParent2);
			CWnd* pParent3 = pParent2->GetParent ();    // Frame (CfDbmsCacheFrame)
			ASSERT_VALID (pParent3);
			if (!((CfDbmsCacheFrame*)pParent3)->IsAllViewsCreated())
				return;
			CvDbmsCacheViewRight* pView = (CvDbmsCacheViewRight*)((CfDbmsCacheFrame*)pParent3)->GetRightPane ();
			ASSERT_VALID (pView);
			CuDlgCacheTab* pTab = pView->GetCacheTabDlg();
			if (pTab)
				pTab->DisplayPage (pData);
		}
		catch (CeVcbfException e)
		{
			//
			// Catch critical error 
			TRACE1 ("CuDlgCacheList::OnItemchangedList1 has caught exception: %s\n", e.m_strReason);
			CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
			pMain->CloseApplication (FALSE);
		}
		catch (...)
		{
			TRACE0 ("CuDlgCacheList::OnUpdateData has caught exception... \n");
		}
	}	
	*pResult = 0;
}

LONG CuDlgCacheList::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return 0L;
	CleanCacheControl (m_ListCtrl);
	m_ListCtrl.DeleteAllItems();

	try
	{
		//
		// Do something low-level: Retrieve data and display it.
		int     iCur, iIndex  = 0;
		BOOL    bCheck = FALSE;
		CString strOnOff;
		VCBFllInitCaches();

		BOOL bContinue = TRUE;
		CACHELINEINFO  data;
		CACHELINEINFO* pData;
		memset (&data, 0, sizeof (data));

		bContinue = VCBFllGetFirstCacheLine(&data);

		while (bContinue)
		{
			strOnOff = data.szstatus;
			strOnOff = strOnOff.Left (2);
			bCheck   = (strOnOff == "ON")? TRUE: FALSE;
			iCur     = m_ListCtrl.InsertItem (iIndex, (LPTSTR)data.szcachename);
			m_ListCtrl.SetCheck   (iIndex, 1, bCheck);
			pData = new CACHELINEINFO;
			memcpy (pData, &data, sizeof (CACHELINEINFO));
			if (iCur != -1)
				m_ListCtrl.SetItemData (iCur, (DWORD_PTR)pData);
			bContinue = VCBFllGetNextCacheLine(&data);
			iIndex++;
		}
		CWnd* pParent1 = GetParent ();              // View (CvDbmsCacheViewLeft)
		ASSERT_VALID (pParent1);
		CWnd* pParent2 = pParent1->GetParent ();    // SplitterWnd
		ASSERT_VALID (pParent2);
		CWnd* pParent3 = pParent2->GetParent ();    // Frame (CfDbmsCacheFrame)
		ASSERT_VALID (pParent3);
		if (!((CfDbmsCacheFrame*)pParent3)->IsAllViewsCreated())
			return 0;
		CvDbmsCacheViewRight* pView = (CvDbmsCacheViewRight*)((CfDbmsCacheFrame*)pParent3)->GetRightPane();
		ASSERT_VALID (pView);
		CuDlgCacheTab* pTab = pView->GetCacheTabDlg();
		if (pTab)
			pTab->DisplayPage (NULL);
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgCacheList::OnUpdateData has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (...)
	{
		TRACE0 ("CuDlgCacheList::OnUpdateData has caught exception... \n");
	}
	return 0L;
}


LONG CuDlgCacheList::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgCacheList::OnButton1...\n");
	return 0L;
}

LONG CuDlgCacheList::OnButton2 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgCacheList::OnButton2...\n");
	return 0L;
}

LONG CuDlgCacheList::OnButton3 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgCacheList::OnButton3...\n");
	return 0L;
}

//
// Cache List Control
// ------------------

CuCacheCheckListCtrl::CuCacheCheckListCtrl()
	:CuListCtrl()
{
}

CuCacheCheckListCtrl::~CuCacheCheckListCtrl()
{
}



BEGIN_MESSAGE_MAP(CuCacheCheckListCtrl, CuListCtrl)

END_MESSAGE_MAP()


void CuCacheCheckListCtrl::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	TRACE1 ("CuDlgDbmsCache::OnCheckChange (%d)\n", (int)bCheck);
	CString strOnOff = bCheck? "ON": "OFF";
	CACHELINEINFO* pData = (CACHELINEINFO*)GetItemData (iItem);
	try
	{
		if (pData)
		{
			CString strOldName = pData->szcachename;
			VCBFllOnCacheEdit (pData, (LPTSTR)(LPCTSTR)strOnOff);
			ASSERT (strOldName == pData->szcachename);
			BOOL bCheck = (pData->szstatus[0] && _tcsicmp ((LPCTSTR)pData->szstatus, _T("ON")) == 0)? TRUE: FALSE;
			SetCheck (iItem, iSubItem,bCheck);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuCacheCheckListCtrl::OnCheckChange has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}
