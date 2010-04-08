/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : brdprdlg.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Protocol Page of Bridge
**
**
**  History:
**	xx-Sep-1997: (uk$so01) 
**	    created
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	16-Oct-2001 (uk$so01)
**	    BUG #106053, Enable/Disable buttons 
**	01-Nov-2001 (somsa01)
**	    Fixed up 64-bit compiler warnings / errors.
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
** 22-Nov-2004 (uk$so01)
**    BUG #113507 / ISSUE #13760338, Update the buttons in the Bridge/protocol Tab.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "brdprdlg.h"
#include "wmusrmsg.h"
#include "crightdg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeProtocol dialog

static void DestroyItemData (CuEditableListCtrlBridge* pList)
{
	BRIDGEPROTLINEINFO* lData = NULL;
	int i, nCount = pList->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		lData = (BRIDGEPROTLINEINFO*)pList->GetItemData (i);
		if (lData)
			delete lData;
	}
}




CuDlgBridgeProtocol::CuDlgBridgeProtocol(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgBridgeProtocol::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgBridgeProtocol)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgBridgeProtocol::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgBridgeProtocol)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgBridgeProtocol, CDialog)
	//{{AFX_MSG_MAP(CuDlgBridgeProtocol)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON3, OnButton3)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON4, OnButton4)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON5, OnButton5)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeProtocol message handlers

void CuDlgBridgeProtocol::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgBridgeProtocol::OnDestroy() 
{
	DestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}

BOOL CuDlgBridgeProtocol::OnInitDialog() 
{
	const int NTAB = 3;
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ImageListCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ListCtrl.SetCheckImageList(&m_ImageListCheck);


	LV_COLUMN lvcolumn;
	UINT strHeaderID[NTAB] = { IDS_COL_PROTOCOL, IDS_COL_STATUS, IDS_COL_LISTEN_ADDRESS};

	int i, hWidth  [NTAB] = {100 , 100, 120};
	int fmt [NTAB] = {LVCFMT_LEFT , LVCFMT_CENTER, LVCFMT_LEFT};
	//
	// Set the number of columns: 3
	//
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	CString csCol;
	for (i=0; i<NTAB; i++)
	{
		lvcolumn.fmt = fmt[i];
		csCol.LoadString(strHeaderID[i]);
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)csCol;
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		if (i==1)
			m_ListCtrl.InsertColumn (i, &lvcolumn, TRUE); // TRUE---> uses checkbox
		else
			m_ListCtrl.InsertColumn (i, &lvcolumn); 
	}
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton2)
		pButton2->ShowWindow(SW_HIDE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgBridgeProtocol::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}


LONG CuDlgBridgeProtocol::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	if (pButton1)
	{
		CString csButtonTitle;
		csButtonTitle.LoadString(IDS_BUTTON_LISTEN_ADDRESS);
		pButton1->SetWindowText (csButtonTitle);

		pButton1->ShowWindow (SW_SHOW);
		pButton1->EnableWindow (FALSE);
	}
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton2)
		pButton2->ShowWindow (SW_HIDE);


	DestroyItemData (&m_ListCtrl);
	m_ListCtrl.DeleteAllItems();
	BOOL bContinue = TRUE;
	BRIDGEPROTLINEINFO  data;
	m_ListCtrl.HideProperty ();
	memset (&data, 0, sizeof (data));

	try
	{
		int nCount, index;
		if (!VCBFll_bridgeprots_init())
			return 0;
		bContinue = VCBFllGetFirstBridgeProtLine(&data);
		while (bContinue)
		{
			CString strVNode = data.szvnode;
			strVNode.TrimLeft();
			strVNode.TrimRight();
			if (!strVNode.IsEmpty()) // Just skip the line that has vnode:
			{
				bContinue = VCBFllGetNextBridgeProtLine(&data);
				continue;
			}
			nCount = m_ListCtrl.GetItemCount();
			//
			// Main Item: col 0
			index = m_ListCtrl.InsertItem (nCount, (LPTSTR)data.szprotocol);
			if (index == -1)
				return 0L;
			//
			// Value: col 1
			if (&data.szstatus[0] && _tcsicmp (_T("ON"), (LPCTSTR)data.szstatus) == 0)
				m_ListCtrl.SetCheck(nCount, 1, TRUE);
			else 
				m_ListCtrl.SetCheck(nCount, 1, FALSE);
			//
			// Listen Address: col 2
			m_ListCtrl.SetItemText (nCount, 2, (LPTSTR)data.szlisten);

			BRIDGEPROTLINEINFO* lData = new BRIDGEPROTLINEINFO;
			memcpy (lData, &data, sizeof(BRIDGEPROTLINEINFO));
			m_ListCtrl.SetItemData (index, (DWORD_PTR)lData);
		
			bContinue = VCBFllGetNextBridgeProtLine(&data);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgBridgeProtocol::OnUpdateData: %s\n", e.m_strReason);
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
	return 0L;
}


LONG CuDlgBridgeProtocol::OnButton1 (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgBridgeProtocol::OnButton1 (Listen Address)...\n");
	CRect r, rCell;
	UINT nState = 0;
	int iNameCol = 2;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_ListCtrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_ListCtrl.EditValue (iIndex, iNameCol, rCell);
	}
	return 0;
}


LONG CuDlgBridgeProtocol::OnButton2(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgBridgeProtocol::OnButton2 (Virtual Node)...\n");
	CRect r, rCell;
	UINT nState = 0;
	int iNameCol = 3;
	int iIndex = -1;
	int i, nCount = m_ListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		nState = m_ListCtrl.GetItemState (i, LVIS_SELECTED);	
		if (nState & LVIS_SELECTED)
		{
			iIndex = i;
			break;
		}
	}
	if (iIndex == -1)
		return 0;
	m_ListCtrl.GetItemRect (iIndex, r, LVIR_BOUNDS);
	BOOL bOk = m_ListCtrl.GetCellRect (r, iNameCol, rCell);
	if (bOk)
	{
		rCell.top    -= 2;
		rCell.bottom += 2;
		m_ListCtrl.EditValue (iIndex, iNameCol, rCell);
	}

	return 0;
}

LONG CuDlgBridgeProtocol::OnButton3(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgBridgeProtocol::OnButton3...\n");
	return 0;
}

LONG CuDlgBridgeProtocol::OnButton4(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgBridgeProtocol::OnButton4...\n");
	return 0;
}

LONG CuDlgBridgeProtocol::OnButton5(UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgBridgeProtocol::OnButton5...\n");
	return 0;
}



/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlBridge

CuEditableListCtrlBridge::CuEditableListCtrlBridge()
	:CuEditableListCtrl()
{
}

CuEditableListCtrlBridge::~CuEditableListCtrlBridge()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlBridge, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlBridge)
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,            OnEditDlgOK)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlBridge message handlers
	
LONG CuEditableListCtrlBridge::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	try
	{
		BRIDGEPROTLINEINFO* pData = (BRIDGEPROTLINEINFO*)GetItemData (iItem);
		if (pData)
		{
			CString strOldName = pData->szprotocol;
			switch (iSubItem)
			{
			case 2:
				VCBFll_bridgeprots_OnEditListen (pData, (LPTSTR)(LPCTSTR)s);
				break;
			case 3:
				VCBFll_bridgeprots_OnEditVnode (pData, (LPTSTR)(LPCTSTR)s);
				break;
			default:
				break;
			}
			ASSERT (strOldName == pData->szprotocol);
			ResetProtocol (iItem, pData);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlBridge::OnEditDlgOK has caught exception: %s\n", e.m_strReason);
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
	return 0L;
}


void CuEditableListCtrlBridge::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	TRACE1 ("CuEditableListCtrlBridge::OnCheckChange (%d)\n", (int)bCheck);
	CString strOnOff = bCheck? "ON": "OFF";
	BRIDGEPROTLINEINFO* pData = (BRIDGEPROTLINEINFO*)GetItemData (iItem);
	try
	{
		if (pData)
		{
			CString strOldName = pData->szprotocol;
			VCBFll_bridgeprots_OnEditStatus (pData, (LPTSTR)(LPCTSTR)strOnOff);
			ASSERT (strOldName == pData->szprotocol);
			ResetProtocol (iItem, pData);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlBridge::OnCheckChange has caught exception: %s\n", e.m_strReason);
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


void CuEditableListCtrlBridge::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber = 1;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	if (rect.PtInRect (point))
		CuListCtrl::OnRButtonDown(nFlags, point);
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 2 && iColumnNumber != 3)
		return;
	EditValue (index, iColumnNumber, rCell);
}



void CuEditableListCtrlBridge::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 2 && iColumnNumber != 3)
		return;
	EditValue (index, iColumnNumber, rCell);
}



void CuEditableListCtrlBridge::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem != 2 && iSubItem != 3)
		return;
	SetEditText   (iItem, iSubItem, rcCell);
}


void CuEditableListCtrlBridge::ResetProtocol (int iItem, BRIDGEPROTLINEINFO* pData)
{
	//
	// Value: col 1
	if (pData->szstatus[0] && _tcsicmp (_T("ON"), (LPCTSTR)pData->szstatus) == 0)
		SetCheck(iItem, 1, TRUE);
	else 
		SetCheck(iItem, 1, FALSE);
	//
	// Listen Address: col 2
	SetItemText (iItem, 2, (LPTSTR)pData->szlisten);
}

void CuDlgBridgeProtocol::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button (Edit Value, Restore)
	BOOL bEnable = (m_ListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);
	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	if (pButton1)
	{
		pButton1->EnableWindow (bEnable);
	}
	*pResult = 0;
}
