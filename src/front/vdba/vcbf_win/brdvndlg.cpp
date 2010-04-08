/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : brdvndlg.cpp : implementation file
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : VNode Page of Bridge
** 
** History:
**
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf, Created 
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
*/

#include "stdafx.h"
#include "vcbf.h"
#include "crightdg.h"
#include "brdvndlg.h"
#include "addvnode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeVNode dialog
static void DestroyItemData (CuListCtrlBridgeVnode* pList)
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

CuDlgBridgeVNode::CuDlgBridgeVNode(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgBridgeVNode::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgBridgeVNode)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgBridgeVNode::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgBridgeVNode)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgBridgeVNode, CDialog)
	//{{AFX_MSG_MAP(CuDlgBridgeVNode)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON1, OnButton1)
	ON_MESSAGE (WM_CONFIGRIGHT_DLG_BUTTON2, OnButton2)
	ON_MESSAGE (WMUSRMSG_CBF_PAGE_UPDATING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeVNode message handlers

void CuDlgBridgeVNode::OnDestroy() 
{
	DestroyItemData (&m_ListCtrl);
	CDialog::OnDestroy();
}

BOOL CuDlgBridgeVNode::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	const int NTAB = 4;
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= WS_CLIPCHILDREN;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 20, TRUE, 1, 0);
	m_ImageListCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ListCtrl.SetCheckImageList(&m_ImageListCheck);


	LVCOLUMN lvcolumn;
	UINT strHeaderID[NTAB] = { IDS_COL_PROTOCOL, IDS_COL_STATUS, IDS_COL_LISTEN_ADDRESS,IDS_COL_VNODE};
	int i, hWidth  [NTAB] = {100 , 100, 120, 100};
	int fmt [NTAB] = {LVCFMT_LEFT , LVCFMT_CENTER, LVCFMT_LEFT, LVCFMT_LEFT};
	//
	// Set the number of columns: 3
	//
	CString csCol;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
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

void CuDlgBridgeVNode::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_ListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_ListCtrl.MoveWindow (r);
}

LONG CuDlgBridgeVNode::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	USES_CONVERSION;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);

	CButton* pButton1 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON1);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if (pButton1 && pButton2)
	{
		CString csButtonTitle;
		csButtonTitle.LoadString(IDS_BUTTON_ADD);
		pButton1->SetWindowText (csButtonTitle);

		csButtonTitle.LoadString(IDS_BUTTON_DELETE);
		pButton2->SetWindowText (csButtonTitle);

		pButton1->ShowWindow (SW_SHOW);
		pButton2->ShowWindow (SW_SHOW);
		pButton1->EnableWindow (TRUE);
		pButton2->EnableWindow (FALSE);
	}

	DestroyItemData (&m_ListCtrl);
	m_ListCtrl.DeleteAllItems();

	try
	{
		int nCount, index;
		if (!VCBFll_bridgeprots_init())
			return 0;
		CPtrList listObj;
		BRIDGE_QueryVNode(listObj);
		POSITION pos = listObj.GetHeadPosition();
		while (pos != NULL)
		{
			BRIDGEPROTLINEINFO* pObj = (BRIDGEPROTLINEINFO*)listObj.GetNext(pos);
			CString strVNode = pObj->szvnode;
			strVNode.TrimLeft();
			strVNode.TrimRight();
			if (strVNode.IsEmpty()) // Just skip the line that does't have vnode:
				continue;
			nCount = m_ListCtrl.GetItemCount();
			//
			// Main Item: col 0
			index = m_ListCtrl.InsertItem (nCount, A2T(pObj->szprotocol));
			if (index == -1)
				return 0L;
			//
			// Value: col 1
			if (&pObj->szstatus[0] && _tcsicmp (_T("ON"), A2T(pObj->szstatus)) == 0)
				m_ListCtrl.SetCheck(nCount, 1, TRUE);
			else 
				m_ListCtrl.SetCheck(nCount, 1, FALSE);
			//
			// Listen Address: col 2
			m_ListCtrl.SetItemText (nCount, 2, A2T(pObj->szlisten));
			//
			// Virtual Node: col 3
			m_ListCtrl.SetItemText (nCount, 3, A2T(pObj->szvnode));
			m_ListCtrl.SetItemData (index, (DWORD_PTR)pObj);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuDlgBridgeVNode::OnUpdateData: %s\n", e.m_strReason);
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

LONG CuDlgBridgeVNode::OnButton1 (UINT wParam, LONG lParam)
{
	USES_CONVERSION;
	CxDlgAddVNode dlg;
	if (dlg.DoModal() == IDOK)
	{
		BOOL bOK = BRIDGE_AddVNode(
		    T2A((LPTSTR)(LPCTSTR)dlg.m_strVNode), 
		    T2A((LPTSTR)(LPCTSTR)dlg.m_strListenAddress), 
		    T2A((LPTSTR)(LPCTSTR)dlg.m_strProtocol));
		if (bOK)
		{
			SendMessage(WMUSRMSG_CBF_PAGE_UPDATING);
		}
	}

	return 0;
}


LONG CuDlgBridgeVNode::OnButton2(UINT wParam, LONG lParam)
{
	USES_CONVERSION;

	UINT nState = 0;
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
	CString strProtocol = m_ListCtrl.GetItemText(iIndex, 0);
	CString strListenAdr= m_ListCtrl.GetItemText(iIndex, 2);
	CString strVNode    = m_ListCtrl.GetItemText(iIndex, 3);
	if (!strProtocol.IsEmpty() && !strListenAdr.IsEmpty() && !strVNode.IsEmpty())
	{
		BOOL bOK = BRIDGE_DelVNode(
		    T2A((LPTSTR)(LPCTSTR)strVNode), 
		    T2A((LPTSTR)(LPCTSTR)strListenAdr), 
		    T2A((LPTSTR)(LPCTSTR)strProtocol));
		if (bOK)
		{
			SendMessage(WMUSRMSG_CBF_PAGE_UPDATING);
		}
	}

	return 0;
}

void CuDlgBridgeVNode::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgBridgeVNode::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	//
	// Enable/Disable Button 
	BOOL bEnable = (m_ListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	CWnd* pParent1 = GetParent ();              // CTabCtrl
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();    // CConfRightDlg
	ASSERT_VALID (pParent2);
	CButton* pButton2 = (CButton*)((CConfRightDlg*)pParent2)->GetDlgItem (IDC_BUTTON2);
	if ( pButton2)
	{
		pButton2->EnableWindow (bEnable);
	}
	*pResult = 0;
}


void CuListCtrlBridgeVnode::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	USES_CONVERSION;
	CString strProtocol = GetItemText(iItem, 0);
	CString strVNode    = GetItemText(iItem, 3);
	BRIDGE_ChangeStatusVNode(T2A((LPTSTR)(LPCTSTR)strVNode), T2A((LPTSTR)(LPCTSTR)strProtocol), bCheck);
}

