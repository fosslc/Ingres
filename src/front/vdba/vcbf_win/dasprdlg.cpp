/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dasprdlg.cpp, Implementation File 
**    Project  : VCBF 
**    Author   : UK Sotheavut
**    Purpose  : Protocol Page of DAS Server
**
**
**  History:
**  10-Mar-2004: (uk$so01) 
**     created
**     SIR #110013 of DAS Server (VCBF should handle DAS Server)
*/

#include "stdafx.h"
#include "vcbf.h"
#include "dasprdlg.h"
#include "wmusrmsg.h"
#include "crightdg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/*
static void DestroyItemData (CuEditableListCtrlDAS* pList)
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
*/

CuDlgDASVRProtocol::CuDlgDASVRProtocol(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDASVRProtocol::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgBridgeProtocol)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgDASVRProtocol::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgBridgeProtocol)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDASVRProtocol, CDialog)
	//{{AFX_MSG_MAP(CuDlgBridgeProtocol)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_VALIDATE, OnValidateData)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnEditListenAddress)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeProtocol message handlers

void CuDlgDASVRProtocol::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgDASVRProtocol::OnDestroy() 
{
	CDialog::OnDestroy();
	ResetList();
}

BOOL CuDlgDASVRProtocol::OnInitDialog() 
{
	const int LAYOUT_NUMBER = 3;
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ImageListCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ListCtrl.SetCheckImageList(&m_ImageListCheck);

	LV_COLUMN lvcolumn;
	UINT strHeaderID[LAYOUT_NUMBER] = { IDS_COL_PROTOCOL, IDS_COL_STATUS, IDS_COL_LISTEN_ADDRESS};
	int i, hWidth   [LAYOUT_NUMBER] = {100 , 100, 100};
	int fmt [LAYOUT_NUMBER] = {LVCFMT_LEFT , LVCFMT_CENTER, LVCFMT_LEFT};
	//
	// Set the number of columns: LAYOUT_NUMBER
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER; i++)
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
	return TRUE;
}

void CuDlgDASVRProtocol::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}


LONG CuDlgDASVRProtocol::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	pButton2->ShowWindow(SW_HIDE);
	CString csButtonTitle;
	csButtonTitle.LoadString(IDS_BUTTON_LISTEN_ADDRESS);
	pButton1->SetWindowText (csButtonTitle);

	pButton1->ShowWindow(SW_SHOW);
	
	//
	// Fill control(s)
	//
	ResetList();
	VCBFll_netprots_init(DAS_PROTOCOL);
	NETPROTLINEINFO protocol;
	BOOL bFirstPass = TRUE;
	while (1) 
	{
		BOOL bOk;
		if (bFirstPass)
		{
			bOk = VCBFllGetFirstNetProtLine(&protocol);
			bFirstPass = FALSE;
		}
		else
			bOk = VCBFllGetNextNetProtLine(&protocol);
		if (!bOk)
			break;    // last item encountered
		if (!AddProtocol(&protocol))
		{
			AfxMessageBox(IDS_ERR_CREATE_PROTO_LIST, MB_ICONSTOP | MB_OK);
			return FALSE;
		}
	}
	return 0L;
}

LONG CuDlgDASVRProtocol::OnValidateData (WPARAM wParam, LPARAM lParam)
{
	m_ListCtrl.HideProperty();
	return 0L;
}

BOOL CuDlgDASVRProtocol::AddProtocol(LPNETPROTLINEINFO lpProtocol)
{
	int nCount, index;

	// fill columns
	nCount = m_ListCtrl.GetItemCount ();
	index  = m_ListCtrl.InsertItem (nCount, (LPCTSTR)lpProtocol->szprotocol);
	if (_tcscmp((LPCTSTR)lpProtocol->szstatus, _T("ON")) == 0) // On/Off
		m_ListCtrl.SetCheck (index, 1, TRUE);
	else
		m_ListCtrl.SetCheck (index, 1, FALSE);
	m_ListCtrl.SetItemText (index, 2, (LPCTSTR)lpProtocol->szlisten);

	// allocate and set data
	LPNETPROTLINEINFO lpDupProtocol = new NETPROTLINEINFO;
	memcpy(lpDupProtocol, lpProtocol, sizeof(NETPROTLINEINFO));
	lpDupProtocol->nProtocolType = DAS_PROTOCOL;
	BOOL bSuccess = m_ListCtrl.SetItemData (index, (DWORD_PTR)lpDupProtocol);

	return bSuccess;
}

void CuDlgDASVRProtocol::ResetList()
{
	int nCount;
	
	nCount = m_ListCtrl.GetItemCount();
	for (int i=0; i<nCount; i++)
	{
		LPNETPROTLINEINFO ptr = (LPNETPROTLINEINFO)m_ListCtrl.GetItemData (i);
		ASSERT (ptr);
		if (ptr)
			delete ptr;
	}
	m_ListCtrl.DeleteAllItems();
}

LONG CuDlgDASVRProtocol::OnEditListenAddress (UINT wParam, LONG lParam)
{
	TRACE0 ("CuDlgDASVRProtocol::OnEditListenAddress...\n");

	// listen address is in column 2
	int iColumn = 2;

	int index = m_ListCtrl.GetCurSel();
	if (index < 0)
		return 0L;

	CRect rItem, rCell;
	m_ListCtrl.GetItemRect (index, rItem, LVIR_BOUNDS);
	BOOL bOk = m_ListCtrl.GetCellRect (rItem, iColumn, rCell);
	if (bOk) 
	{
		m_ListCtrl.HideProperty();
		m_ListCtrl.SetEditText (index, iColumn, rCell);
	}

	return 0L;
}





/*
/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlBridge

CuEditableListCtrlDAS::CuEditableListCtrlDAS()
	:CuEditableListCtrl()
{
}

CuEditableListCtrlDAS::~CuEditableListCtrlDAS()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlDAS, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlDAS)
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,            OnEditDlgOK)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlBridge message handlers
	
LONG CuEditableListCtrlDAS::OnEditDlgOK (UINT wParam, LONG lParam)
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
		TRACE1 ("CuEditableListCtrlDAS::OnEditDlgOK has caught exception: %s\n", e.m_strReason);
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


void CuEditableListCtrlDAS::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	TRACE1 ("CuEditableListCtrlDAS::OnCheckChange (%d)\n", (int)bCheck);
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
		TRACE1 ("CuEditableListCtrlDAS::OnCheckChange has caught exception: %s\n", e.m_strReason);
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


void CuEditableListCtrlDAS::OnRButtonDown(UINT nFlags, CPoint point) 
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



void CuEditableListCtrlDAS::OnLButtonDblClk(UINT nFlags, CPoint point) 
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



void CuEditableListCtrlDAS::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	if (iSubItem != 2 && iSubItem != 3)
		return;
	SetEditText   (iItem, iSubItem, rcCell);
}


void CuEditableListCtrlDAS::ResetProtocol (int iItem, BRIDGEPROTLINEINFO* pData)
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
*/

void CuDlgDASVRProtocol::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton1 && pButton2)
	{
		pButton1->EnableWindow (bEnable);
		pButton2->EnableWindow (bEnable);
	}
	*pResult = 0;
}
