/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.cpp : implementation file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : implementation of the Frame CfMainFrame class  (MFC frame/view/doc).
**
** History:
**	10-Dec-2001 (uk$so01)
**	    Created
**	26-Feb-2003 (uk$so01)
**	    SIR #106648, conform to the change of DDS VDBA split
**	19-May-2003 (uk$so01)
**	    SIR #110269, Add Network trafic monitoring.
**	03-Oct-2003 (uk$so01)
**	    SIR #106648, Vdba-Split, Additional fix for GATEWAY Enhancement 
**	30-Jan-2004 (uk$so01)
**	    SIR #111701, Use Compiled HTML Help (.chm file)
**	02-feb-2004 (somsa01)
**	    In CfMainFrame::OnHelpInfo(), use the non-class version of
**	    HtmlHelp(). Also updated old CMemoryException's which are now
**	    errors.
** 16-Mar-2004 (uk$so01)
**    BUG #111965 / ISSUE 13296981 The Vdbamon's menu item "Help Topic" is disabled.
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 04-Jun-2004 (uk$so01)
**    SIR #111701, Connect Help to History of SQL Statement error.
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should always invoke the topic Overview if no ocx control is shown.
**    Otherwise F1-key should invoke always the ocx.s help.
** 21-Aug-2008 (whiro01)
**    Removed redundant <afx...> include which is already in "stdafx.h"
*/

#include "stdafx.h"
#include <comdef.h>
#include <htmlhelp.h>
#include "rcdepend.h"
#include "xipm.h"
#include "mainfrm.h"
#include "xipmdml.h"
#include "ipmdoc.h"
#include "dmluser.h"
#include "rctools.h"
#include "usrexcep.h"
#include "compfile.h"
#include "sqlerror.h"
#include ".\mainfrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define _NO_OPENSAVE
//#define _MINI_CACHE
#define REFRESH_NODE    0x00000001
#define REFRESH_SERVER  0x00000002
#define REFRESH_USER    0x00000004


IMPLEMENT_DYNCREATE(CfMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CfMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(IDM_GO, OnGo)
	ON_COMMAND(IDM_REFRESH, OnForceRefresh)
	ON_COMMAND(IDM_SHUT_DOWN, OnShutDown)
	ON_COMMAND(IDM_EXPAND_BRANCH, OnExpandBranch)
	ON_COMMAND(IDM_EXPAND_ALL, OnExpandAll)
	ON_COMMAND(IDM_COLLAPSE_BRANCH, OnCollapseBranch)
	ON_COMMAND(IDM_COLLAPSE_ALL, OnCollapseAll)
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(IDM_EXPAND_BRANCH, OnUpdateExpandBranch)
	ON_UPDATE_COMMAND_UI(IDM_EXPAND_ALL, OnUpdateExpandAll)
	ON_UPDATE_COMMAND_UI(IDM_COLLAPSE_BRANCH, OnUpdateCollapseBranch)
	ON_UPDATE_COMMAND_UI(IDM_COLLAPSE_ALL, OnUpdateCollapseAll)
	ON_UPDATE_COMMAND_UI(IDM_SHUT_DOWN, OnUpdateShutDown)
	ON_UPDATE_COMMAND_UI(IDM_REFRESH, OnUpdateForceRefresh)
	ON_COMMAND(IDM_OPEN_ENVIRONMENT, OnFileOpen)
	ON_COMMAND(IDM_SAVE_ENVIRONMENT, OnFileSave)
	ON_UPDATE_COMMAND_UI(IDM_OPEN_ENVIRONMENT, OnUpdateFileOpen)
	ON_UPDATE_COMMAND_UI(IDM_SAVE_ENVIRONMENT, OnUpdateFileSave)
	ON_COMMAND(ID_VIEW_TOOLBAR2, OnViewToolbar)
	ON_WM_SIZE()
	ON_COMMAND(IDM_PREFERENCE, OnPreference)
	ON_UPDATE_COMMAND_UI(IDM_PREFERENCE, OnUpdatePreference)
	ON_UPDATE_COMMAND_UI(IDM_PREFERENCE_SAVE, OnUpdatePreferenceSave)
	ON_COMMAND(IDM_PREFERENCE_SAVE, OnPreferenceSave)
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI(IDM_GO, OnUpdateGo)
	ON_COMMAND(ID_SQL_ERROR, OnHistoricSqlError)
	ON_UPDATE_COMMAND_UI(ID_SQL_ERROR, OnUpdateHistoricSqlError)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP

	//
	// Virtual Node Combo notification:
	ON_CBN_DROPDOWN (IDM_NODE, OnDropDownNode)
	ON_CBN_SELCHANGE(IDM_NODE, OnSelChangeNode)
	//
	// Server Class Combo notification:
	ON_CBN_DROPDOWN (IDM_SERVER_CLASS, OnDropDownServer)
	ON_CBN_SELCHANGE(IDM_SERVER_CLASS, OnSelChangeServer)
	//
	// User Combo notification:
	ON_CBN_DROPDOWN (IDM_USER, OnDropDownUser)
	ON_CBN_SELCHANGE(IDM_USER, OnSelChangeUser)

	// 
	// Dialogbar notification:
	// **********************
	// Resource Type Combo notification:
	ON_CBN_SELCHANGE(IDM_RESOURCE_TYPE,           OnSelChangeResourceType)
	ON_BN_CLICKED   (IDM_NULL_RESOURCE,           OnCheckNullResource)
	ON_BN_CLICKED   (IDM_INTERNAL_SESSION,        OnCheckInternalSession)
	ON_BN_CLICKED   (IDM_INACTIVE_TRANSACTION,    OnCheckInactiveTransaction)
	ON_BN_CLICKED   (IDM_SYSTEM_LOCKLIST,         OnCheckSystemLockList)
	ON_BN_CLICKED   (IDM_EXPRESS_REFRESH,         OnCheckExpresRefresh)
	ON_EN_CHANGE    (IDM_REFRESH_FREQUENCY,       OnEditChangeFrequency)

	ON_UPDATE_COMMAND_UI(IDM_RESOURCE_TYPE,       OnUpdateResourceType)
	ON_UPDATE_COMMAND_UI(IDM_NULL_RESOURCE,       OnUpdateNullResource)
	ON_UPDATE_COMMAND_UI(IDM_INTERNAL_SESSION,    OnUpdateInternalSession)
	ON_UPDATE_COMMAND_UI(IDM_INACTIVE_TRANSACTION,OnUpdateInactiveTransaction)
	ON_UPDATE_COMMAND_UI(IDM_SYSTEM_LOCKLIST,     OnUpdateSystemLockList)
	ON_UPDATE_COMMAND_UI(IDM_EXPRESS_REFRESH,     OnUpdateExpresRefresh)
	ON_UPDATE_COMMAND_UI(IDM_REFRESH_FREQUENCY,   OnUpdateEditFrequency)
	ON_COMMAND(ID_DEFAULT_HELP, OnDefaultHelp)
END_MESSAGE_MAP()


static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


/////////////////////////////////////////////////////////////////////////////
// CfMainFrame construction/destruction

CfMainFrame::CfMainFrame()
{
	m_bCommandLine = FALSE;
	m_bPropertyChange = FALSE;
	m_nAlreadyRefresh = 0;
	m_strDefaultServer.LoadString(IDS_DEFAULT_SERVER);
	m_strDefaultUser.LoadString(IDS_DEFAULT_USER);

	m_pDefaultServer = new CaDBObject (m_strDefaultServer, _T(""));
	m_pDefaultUser   = new CaDBObject (m_strDefaultUser, _T(""));

	m_bSavePreferenceAsDefault = TRUE;
	m_bActivateApply = TRUE;
}

CfMainFrame::~CfMainFrame()
{
	if (m_pDefaultServer)
		delete m_pDefaultServer;
	if (m_pDefaultUser)
		delete m_pDefaultUser;
	while (!m_lNode.IsEmpty())
		delete m_lNode.RemoveHead();
	while (!m_lServer.IsEmpty())
		delete m_lServer.RemoveHead();
	while (!m_lUser.IsEmpty())
		delete m_lUser.RemoveHead();
}

void CfMainFrame::UpdateLoadedData(CdIpm* pDoc, FRAMEDATASTRUCT* pFrameData)
{
	m_nAlreadyRefresh = 0;
	m_pComboNode   = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
	m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
	m_pComboUser   = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
	m_pComboNode->ResetContent();
	m_pComboServer->ResetContent();
	m_pComboUser->ResetContent();

	int nIdx = CB_ERR;
	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	POSITION pos = NULL;

	//
	// Update node combobox:
	pos = m_lNode.GetHeadPosition();
	while (pos != NULL)
	{
		CaNode* pNode = (CaNode*)m_lNode.GetNext(pos);
		if (pNode->IsLocalNode())
		{
			cbitem.iImage = 1;
			cbitem.iSelectedImage = 1;
		}
		else
		{
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
		}

		cbitem.pszText = (LPTSTR)(LPCTSTR)pNode->GetName();
		cbitem.lParam  = (LPARAM)pNode;
		cbitem.iItem   = m_pComboNode->GetCount();
		nIdx = m_pComboNode->InsertItem (&cbitem);
	}
	nIdx = m_pComboNode->FindStringExact(-1, pDoc->GetNode());
	if (nIdx != CB_ERR)
		m_pComboNode->SetCurSel(nIdx);
	else
		m_pComboNode->SetCurSel(0);

	//
	// Update server combobox:
	SetDefaultServer();
	pos = m_lServer.GetHeadPosition();
	while (pos != NULL)
	{
		CaNodeServer* pNodeServer = (CaNodeServer*)m_lServer.GetNext(pos);
		cbitem.pszText = (LPTSTR)(LPCTSTR)pNodeServer->GetName();
		cbitem.lParam  = (LPARAM)pNodeServer;
		cbitem.iItem   = m_pComboServer->GetCount();
		m_pComboServer->InsertItem (&cbitem);
	}
	nIdx = m_pComboServer->FindStringExact(-1, pDoc->GetServer());
	if (nIdx != CB_ERR)
		m_pComboServer->SetCurSel(nIdx);
	else
		m_pComboServer->SetCurSel(0);

	//
	// Update User combobox:
	SetDefaultUser();
	pos = m_lUser.GetHeadPosition();
	while (pos != NULL)
	{
		CaUser* pUser = (CaUser*)m_lUser.GetNext(pos);
		cbitem.pszText = (LPTSTR)(LPCTSTR)pUser->GetName();
		cbitem.lParam  = (LPARAM)pUser;
		cbitem.iItem   = m_pComboUser->GetCount();
		m_pComboUser->InsertItem (&cbitem);
	}
	nIdx = m_pComboUser->FindStringExact(-1, pDoc->GetUser());
	if (nIdx != CB_ERR)
		m_pComboUser->SetCurSel(nIdx);
	else
		m_pComboUser->SetCurSel(0);

	//
	// Update Filters:
	ASSERT(pFrameData);
	if (!pFrameData)
		return;
	ASSERT(pFrameData->nDim == 5);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_NULL_RESOURCE))->SetCheck(pFrameData->arrayFilter[0]);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_INTERNAL_SESSION))->SetCheck(pFrameData->arrayFilter[1]);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_SYSTEM_LOCKLIST))->SetCheck(pFrameData->arrayFilter[2]);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_INACTIVE_TRANSACTION))->SetCheck(pFrameData->arrayFilter[3]);

	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem(IDM_RESOURCE_TYPE);
	int i, nCount = pCombo->GetCount();
	for (i=0; i<nCount; i++)
	{
		if (pFrameData->arrayFilter[4] == (short)pCombo->GetItemData(i))
		{
			pCombo->SetCurSel(i);
			break;
		}
	}

	//
	// Update Express Refresh Frequency:
	CString strFrequency;
	strFrequency.Format(_T("%d"), pDoc->GetExpressRefreshFrequency());
	((CEdit*)m_wndDlgBar.GetDlgItem(IDM_REFRESH_FREQUENCY))->SetWindowText (strFrequency);
}


void CfMainFrame::ShowIpmControl(BOOL bShow)
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	ASSERT(pIpmDoc);
	if (!pIpmDoc)
		return;
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	ASSERT(pIpmCtrl);
	if (!pIpmCtrl)
		return;
	if (!IsWindow (pIpmCtrl->m_hWnd))
		return;
	pIpmCtrl->ShowWindow (bShow? SW_SHOW: SW_HIDE);
}

int CfMainFrame::SetDefaultServer(BOOL bCleanCombo)
{
	m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
	if (bCleanCombo)
	{
		while (!m_lServer.IsEmpty())
			delete m_lServer.RemoveHead();
		m_pComboServer->ResetContent();
	}
	
	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	cbitem.pszText = (LPTSTR)(LPCTSTR)m_pDefaultServer->GetName();
	cbitem.lParam  = (LPARAM)m_pDefaultServer;
	cbitem.iItem   = m_pComboServer->GetCount();
	int nSel = m_pComboServer->InsertItem (&cbitem);
	m_pComboServer->SetCurSel(nSel);
	return nSel;
}


int CfMainFrame::SetDefaultUser(BOOL bCleanCombo)
{
	m_pComboUser = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
	if (bCleanCombo)
	{
		while (!m_lUser.IsEmpty())
			delete m_lUser.RemoveHead();
		m_pComboUser->ResetContent();
	}

	COMBOBOXEXITEM cbitem;
	memset (&cbitem, 0, sizeof(cbitem));
	cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
	cbitem.iImage = 0;
	cbitem.iSelectedImage = 0;
	cbitem.pszText = (LPTSTR)(LPCTSTR)m_pDefaultUser->GetName();
	cbitem.lParam  = (LPARAM)m_pDefaultUser;
	cbitem.iItem   = m_pComboUser->GetCount();
	int nSel = m_pComboUser->InsertItem (&cbitem);
	m_pComboUser->SetCurSel(nSel);
	return nSel;
}


int CfMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int i;
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//
	// Create the ConnectBar (DialogBar)
	if (!m_wndDlgConnectBar.Create(this, IDR_CONNECTBAR, CBRS_ALIGN_ANY, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1; // fail to create
	}
	m_ImageListNode.Create(IDB_NODES, 16, 1, RGB(255, 0, 255));
	m_ImageListServer.Create(IDB_SERVER_OTHER, 16, 1, RGB(255, 0, 255));
	m_ImageListUser.Create(IDB_USER, 16, 1, RGB(255, 0, 255));
	m_pComboNode   = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
	m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
	m_pComboUser   = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
	ASSERT (m_pComboNode && m_pComboServer && m_pComboUser);
	if (!m_pComboNode || !m_pComboServer || !m_pComboUser)
		return -1;
	m_pComboNode->SetImageList(&m_ImageListNode);
	m_pComboServer->SetImageList(&m_ImageListServer);
	m_pComboUser->SetImageList(&m_ImageListUser);
	//
	// Create the Toolbar (command)
	if (!m_wndToolBarCmd.CreateEx(this) || !m_wndToolBarCmd.LoadToolBar(IDR_IPMCOMMAND))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

#if defined (_TOOTBAR_BTN_TEXT_)
	//
	// Set the Button Text:
	CString strTips;
	UINT nID = 0;
	int nButtonCount = m_wndToolBarCmd.GetToolBarCtrl().GetButtonCount();
	for (i=0; i<nButtonCount; i++)
	{
		if (m_wndToolBarCmd.GetButtonStyle(i) & TBBS_SEPARATOR)
			continue;
		nID = m_wndToolBarCmd.GetItemID(i);
		if (strTips.LoadString(nID))
		{
			int nFound = strTips.Find (_T("\n"));
			if (nFound != -1)
				strTips = strTips.Mid (nFound+1);
			m_wndToolBarCmd.SetButtonText(i, strTips);
		}
	}
	CRect temp;
	m_wndToolBarCmd.GetItemRect(0,&temp);
	m_wndToolBarCmd.SetSizes( CSize(temp.Width(),temp.Height()), CSize(16,16));
#endif
#if defined (_TOOTBAR_HOT_IMAGE_)
	//
	// Set the Hot Image:
	CToolBarCtrl& tbctrl = m_wndToolBarCmd.GetToolBarCtrl();
	CImageList image;
	image.Create(IDB_HOTMAINFRAME, 16, 0, RGB (128, 128, 128));
	tbctrl.SetHotImageList (&image);
	image.Detach();
#endif

	//
	// Create the DialogBar (for Filter):
	if (!m_wndDlgBar.Create(this, IDR_FILTER, CBRS_ALIGN_ANY, 5000))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1; // fail to create
	}

	if (!m_wndReBar.Create(this) ||
	    !m_wndReBar.AddBar(&m_wndDlgConnectBar) ||
	    !m_wndReBar.AddBar(&m_wndToolBarCmd) ||
	    !m_wndReBar.AddBar(&m_wndDlgBar))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	UINT nExtraStyle = CBRS_FLYBY | CBRS_TOOLTIPS | CBRS_SIZE_DYNAMIC | /*CBRS_GRIPPER |*/ CBRS_BORDER_3D;
	m_wndDlgBar.SetBarStyle(m_wndToolBarCmd.GetBarStyle() | nExtraStyle);
	m_wndToolBarCmd.SetBarStyle(m_wndToolBarCmd.GetBarStyle() | nExtraStyle);
	m_wndDlgConnectBar.SetBarStyle(m_wndDlgConnectBar.GetBarStyle() | nExtraStyle);

	CReBarCtrl& bctrl = m_wndReBar.GetReBarCtrl();
	bctrl.MaximizeBand(1);
	m_wndReBar.Invalidate();

	CdIpm* pDoc = GetIpmDoc();
	ASSERT(pDoc);
	int nIdx = CB_ERR;
	OnDropDownNode();

	SetDefaultServer();
	SetDefaultUser();

	CComboBox* pComboResource = (CComboBox*)m_wndDlgBar.GetDlgItem (IDM_RESOURCE_TYPE);
	ASSERT(pComboResource);
	if (pComboResource)
	{
		DML_FillComboResourceType(pComboResource);
	}
	CString strFrequency;
	strFrequency.Format(_T("%d"), pDoc->GetExpressRefreshFrequency());
	((CEdit*)m_wndDlgBar.GetDlgItem(IDM_REFRESH_FREQUENCY))->SetWindowText (strFrequency);
	
	CString strItem;
	int nSel = m_pComboNode->GetCurSel();
	if (pDoc && nSel != CB_ERR)
	{
		BOOL bInitiate = FALSE;
		m_pComboNode->GetLBText(nSel, strItem);
		if (!strItem.IsEmpty())
		{
			pDoc->SetNode(strItem);
			bInitiate = TRUE;
		}
		m_pComboServer->GetLBText(0, strItem);
		pDoc->SetServer(strItem);
		m_pComboUser->GetLBText(0, strItem);
		pDoc->SetUser(strItem);
	}

	//
	// Check the menu view toolbar.
	CMenu* pMenu = GetMenu();
	if (!pMenu)
		return 0;
	UINT nState = pMenu->GetMenuState(ID_VIEW_TOOLBAR2, MF_BYCOMMAND);
	if (!(nState & MF_CHECKED))
		pMenu->CheckMenuItem(ID_VIEW_TOOLBAR2, MF_CHECKED|MF_BYCOMMAND);

	PermanentState(TRUE);

	//
	// Handle the command lines:
	HandleCommandLine(pDoc);

#if defined (_NO_OPENSAVE)
	CMenu* pMainMenu = GetMenu();
	if (pMainMenu)
	{
		pMainMenu->DeleteMenu(IDM_OPEN_ENVIRONMENT, MF_BYCOMMAND);
		pMainMenu->DeleteMenu(IDM_SAVE_ENVIRONMENT, MF_BYCOMMAND);
	}
#endif

	return 0;
}

BOOL CfMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CfMainFrame diagnostics

#ifdef _DEBUG
void CfMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CfMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

void CfMainFrame::GetFilters(short* arrayFilter, int nSize)
{
	ASSERT(nSize >= 5);
	for (int i=0; i<nSize; i++)
		arrayFilter[i] = 0;
	arrayFilter[0] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_NULL_RESOURCE))->GetCheck();
	arrayFilter[1] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_INTERNAL_SESSION))->GetCheck();
	arrayFilter[2] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_SYSTEM_LOCKLIST))->GetCheck();
	arrayFilter[3] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_INACTIVE_TRANSACTION))->GetCheck();
	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem(IDM_RESOURCE_TYPE);
	int nSel = pCombo->GetCurSel();
	arrayFilter[4] = (short)pCombo->GetItemData(nSel);
}


BOOL CfMainFrame::ShouldEnable()
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	ASSERT(pIpmDoc);
	if (!pIpmDoc)
		return FALSE;
	BOOL bConnected = pIpmDoc->IsConnected();
	return bConnected;
}

void CfMainFrame::DisableExpresRefresh()
{
	TRACE0("CfMainFrame::DisableExpresRefresh\n");
	CButton* pButton = (CButton*)m_wndDlgBar.GetDlgItem(IDM_EXPRESS_REFRESH);
	short nCheck = (short)pButton->GetCheck();
	if (nCheck==1)
	{
		pButton->SetCheck(0);
		OnCheckExpresRefresh();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CfMainFrame message handlers
void CfMainFrame::OnGo() 
{
	CWaitCursor doWaitCursor;
	CdIpm* pDoc = GetIpmDoc();
	pDoc->UpdateAllViews(NULL, 2);
	if (pDoc->IsConnected())
		ShowIpmControl(TRUE);
}

void CfMainFrame::OnForceRefresh() 
{
	m_nAlreadyRefresh = 0;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (pIpmCtrl->IsWindowVisible())
	{
		pIpmCtrl->ForceRefresh();
	}
}

void CfMainFrame::OnShutDown() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->ItemShutdown();
}

void CfMainFrame::OnExpandBranch() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->ExpandBranch();
	
}

void CfMainFrame::OnExpandAll() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->ExpandAll();
}

void CfMainFrame::OnCollapseBranch() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->CollapseBranch();
}

void CfMainFrame::OnCollapseAll() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->CollapseAll();
}


BOOL CfMainFrame::QueryNode()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	BOOL bOK = FALSE;
	try
	{
		m_pComboNode = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
		if (!m_pComboNode)
			return FALSE;
		CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
		CString strNode = pIpmDoc->GetNode();
#if defined (_MINI_CACHE)
		if (m_nAlreadyRefresh & REFRESH_NODE)
			return TRUE; // Nodes have been queried !
#endif
		CTypedPtrList< CObList, CaDBObject* > lNew;
		bOK = DML_QueryNode(pIpmDoc, lNew);
		if (bOK)
		{
			while (!m_lNode.IsEmpty())
				delete m_lNode.RemoveHead();
			m_pComboNode->ResetContent();

			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaNode* pNode = (CaNode*)lNew.RemoveHead();
				if (pNode->IsLocalNode())
				{
					cbitem.iImage = 1;
					cbitem.iSelectedImage = 1;
				}
				else
				{
					cbitem.iImage = 0;
					cbitem.iSelectedImage = 0;
				}

				cbitem.pszText = (LPTSTR)(LPCTSTR)pNode->GetName();
				cbitem.lParam  = (LPARAM)pNode;
				cbitem.iItem   = m_pComboNode->GetCount();
				nIdx = m_pComboNode->InsertItem (&cbitem);
				if (pNode->IsLocalNode())
					nLocal = nIdx;

				m_lNode.AddTail(pNode);
			}

			if (!strNode.IsEmpty())
			{
				nIdx = m_pComboNode->FindStringExact(-1, strNode);
				if (nIdx != CB_ERR)
					m_pComboNode->SetCurSel(nIdx);
				else
					m_pComboNode->SetCurSel(0);
			}
			else
			if (nLocal != -1)
			{
				m_pComboNode->SetCurSel (nLocal);
			}
			else
				m_pComboNode->SetCurSel (0);

			m_nAlreadyRefresh |= REFRESH_NODE;
			
			SetDefaultServer(TRUE);
			SetDefaultUser(TRUE);
		}
	}
	catch (CeNodeException e)
	{
		SetForegroundWindow();
		BOOL bStarted = INGRESII_IsRunning();
		if (!bStarted)
			AfxMessageBox (IDS_MSG_INGRES_NOT_START);
		else
			AfxMessageBox (e.GetReason());
		bOK=FALSE;
	}
	catch(...)
	{
		bOK=FALSE;
	}

	return bOK;
}

BOOL CfMainFrame::QueryServer()
{
	BOOL bOK = FALSE;
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
#if defined (_MINI_CACHE)
	if (m_nAlreadyRefresh & REFRESH_SERVER)
		return TRUE; // Servers have been queried !
#endif
	try
	{
		CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
		m_pComboNode = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
		if (!m_pComboNode)
			return FALSE;
		m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
		if (!m_pComboServer)
			return FALSE;
		nIdx = m_pComboNode->GetCurSel();
		if (nIdx == CB_ERR)
			return FALSE;

		CString strServer = pIpmDoc->GetServer();
		CTypedPtrList< CObList, CaDBObject* > lNew;
		CaNode* pNode = (CaNode*)m_pComboNode->GetItemDataPtr(nIdx);
		bOK = DML_QueryServer(pIpmDoc, pNode, lNew);
		if (bOK)
		{
			while (!m_lServer.IsEmpty())
				delete m_lServer.RemoveHead();
			m_pComboServer->ResetContent();

			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
			int nDefault = SetDefaultServer();

			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaNodeServer* pNodeServer = (CaNodeServer*)lNew.RemoveHead();
				cbitem.pszText = (LPTSTR)(LPCTSTR)pNodeServer->GetName();
				cbitem.lParam  = (LPARAM)pNodeServer;
				cbitem.iItem   = m_pComboServer->GetCount();
				m_pComboServer->InsertItem (&cbitem);

				m_lServer.AddTail(pNodeServer);
			}

			if (!strServer.IsEmpty())
			{
				nIdx = m_pComboServer->FindStringExact(-1, strServer);
				if (nIdx != CB_ERR)
					m_pComboServer->SetCurSel(nIdx);
				else
					m_pComboServer->SetCurSel(nDefault);
			}
			else
			{
				m_pComboServer->SetCurSel(nDefault);
			}
			m_nAlreadyRefresh |= REFRESH_SERVER;
		}
	}
	catch (CeNodeException e)
	{
		bOK = FALSE;
		SetForegroundWindow();
		BOOL bStarted = INGRESII_IsRunning();
		if (!bStarted)
			AfxMessageBox (IDS_MSG_INGRES_NOT_START);
		else
			AfxMessageBox (e.GetReason());
	}
	catch(...)
	{
		bOK = FALSE;
	}
	return bOK;
}

BOOL CfMainFrame::QueryUser()
{
	BOOL bOK = FALSE;
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
#if defined (_MINI_CACHE)
	if (m_nAlreadyRefresh & REFRESH_USER)
		return TRUE; // Users have been queried !
#endif
	try
	{
		m_pComboNode = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
		if (!m_pComboNode)
			return FALSE;
		m_pComboUser = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
		if (!m_pComboUser)
			return FALSE;
		nIdx = m_pComboNode->GetCurSel();
		if (nIdx == CB_ERR)
			return FALSE;

		CString strUser = pIpmDoc->GetUser();
		CTypedPtrList< CObList, CaDBObject* > lNew;
		bOK = DML_QueryUser(pIpmDoc, lNew);
		if (bOK)
		{
			while (!m_lUser.IsEmpty())
				delete m_lUser.RemoveHead();
			m_pComboUser->ResetContent();

			COMBOBOXEXITEM cbitem;
			memset (&cbitem, 0, sizeof(cbitem));
			cbitem.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM ;
			cbitem.iImage = 0;
			cbitem.iSelectedImage = 0;
			int nDefault = SetDefaultUser();

			POSITION pos = lNew.GetHeadPosition();
			while (!lNew.IsEmpty())
			{
				CaUser* pUser = (CaUser*)lNew.RemoveHead();
				cbitem.pszText = (LPTSTR)(LPCTSTR)pUser->GetName();
				cbitem.lParam  = (LPARAM)pUser;
				cbitem.iItem   = m_pComboUser->GetCount();
				m_pComboUser->InsertItem (&cbitem);

				m_lUser.AddTail(pUser);
			}
			if (!strUser.IsEmpty())
			{
				nIdx = m_pComboUser->FindStringExact(-1, strUser);
				if (nIdx != CB_ERR)
					m_pComboUser->SetCurSel(nIdx);
				else
					m_pComboUser->SetCurSel(nDefault);
			}
			else
			{
				m_pComboUser->SetCurSel(nDefault);
			}
			m_nAlreadyRefresh |= REFRESH_USER;
		}
	}
	catch (CeSqlException e)
	{
		SetForegroundWindow();
		CString csTemp,csStatement;
		csTemp.Format(_T("%ld"),e.GetErrorCode());
		csStatement.LoadString(IDS_S_UNKNOWN_STATEMENT);//"Unknown statement - statistic data only"
		pIpmDoc->GetClassFilesErrors()->WriteSqlErrorInFiles(e.GetReason(),csStatement,csTemp);
		if (e.GetErrorCode() == -30140)
		{
			AfxMessageBox (IDS_MSG_USER_UNAVAILABLE);
			return FALSE;
		}
		AfxMessageBox (e.GetReason());
		bOK = FALSE;
	}
	catch(...)
	{
		bOK = FALSE;
	}

	return bOK;
}



void CfMainFrame::OnDropDownNode()
{
	QueryNode();
}

void CfMainFrame::OnSelChangeNode()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CString strNode = pIpmDoc->GetNode();
	m_pComboNode = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
	if (!m_pComboNode)
		return;
	CString strNewNode;
	nIdx = m_pComboNode->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboNode->GetLBText (nIdx, strNewNode);
	if (strNewNode.CompareNoCase(strNode) != 0)
	{
		pIpmDoc->SetNode(strNewNode);
		ShowIpmControl(FALSE);
		m_nAlreadyRefresh &=~REFRESH_SERVER;
		m_nAlreadyRefresh &=~REFRESH_USER;
		SetDefaultServer(TRUE);
		SetDefaultUser(TRUE);
		m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
		if (!m_pComboServer)
			return;
		m_pComboUser = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
		if (!m_pComboUser)
			return;
		CString strNewServer;
		CString strNewUser;
		nIdx = m_pComboServer->GetCurSel();
		if (nIdx != CB_ERR)
		{
			m_pComboServer->GetLBText (nIdx, strNewServer);
			pIpmDoc->SetServer(strNewServer);
		}
		nIdx = m_pComboUser->GetCurSel();
		if (nIdx != CB_ERR)
		{
			m_pComboUser->GetLBText (nIdx, strNewUser);
			pIpmDoc->SetUser(strNewUser);
		}

		pIpmDoc->UpdateAllViews(NULL, 4);
	}
}

void CfMainFrame::OnDropDownServer()
{
	QueryServer();
}

void CfMainFrame::OnSelChangeServer()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CString strServer = pIpmDoc->GetServer();
	CString strNewServer;
	m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
	if (!m_pComboServer)
		return;
	nIdx = m_pComboServer->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboServer->GetLBText (nIdx, strNewServer);
	if (strNewServer.CompareNoCase(strServer) != 0)
	{
		pIpmDoc->SetServer(strNewServer);
		ShowIpmControl(FALSE);
		m_nAlreadyRefresh &=~REFRESH_USER;
		SetDefaultUser(TRUE);
		CString strNewUser;
		nIdx = m_pComboUser->GetCurSel();
		if (nIdx != CB_ERR)
		{
			m_pComboUser->GetLBText (nIdx, strNewUser);
			pIpmDoc->SetUser(strNewUser);
		}

		pIpmDoc->UpdateAllViews(NULL, 4);
	}
}

void CfMainFrame::OnDropDownUser()
{
	QueryUser();
}



void CfMainFrame::OnSelChangeUser()
{
	CWaitCursor doWaitCursor;
	int nIdx = CB_ERR;
	int nLocal = -1;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CString strUser = pIpmDoc->GetUser();

	CString strNewUser;
	m_pComboUser = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
	if (!m_pComboUser)
		return;
	nIdx = m_pComboUser->GetCurSel();
	if (nIdx == CB_ERR)
		return;
	m_pComboUser->GetLBText (nIdx, strNewUser);
	if (strNewUser.CompareNoCase(strUser) != 0)
	{
		pIpmDoc->SetUser(strNewUser);
		ShowIpmControl(FALSE);
	}
}

void CfMainFrame::OnSelChangeResourceType()
{
	TRACE0("CfMainFrame::OnSelChangeResourceType\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem(IDM_RESOURCE_TYPE);
	int nSel = pCombo->GetCurSel();
	short nResType = (short)pCombo->GetItemData(nSel);
	pIpmCtrl->ResourceTypeChange(nResType);
}

void CfMainFrame::OnCheckNullResource()
{
	TRACE0("CfMainFrame::OnCheckNullResource\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_NULL_RESOURCE))->GetCheck();
	pIpmCtrl->NullResource(nCheck);
}

void CfMainFrame::OnCheckInternalSession()
{
	TRACE0("CfMainFrame::OnCheckInternalSession\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_INTERNAL_SESSION))->GetCheck();
	pIpmCtrl->InternalSession(nCheck);
}

void CfMainFrame::OnCheckInactiveTransaction()
{
	TRACE0("CfMainFrame::OnCheckInactiveTransaction\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_INACTIVE_TRANSACTION))->GetCheck();
	pIpmCtrl->InactiveTransaction(nCheck);
}

void CfMainFrame::OnCheckSystemLockList()
{
	TRACE0("CfMainFrame::OnCheckSystemLockList\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_SYSTEM_LOCKLIST))->GetCheck();
	pIpmCtrl->SystemLockList(nCheck);
}

void CfMainFrame::OnCheckExpresRefresh()
{
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_EXPRESS_REFRESH))->GetCheck();
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (pIpmDoc && pIpmCtrl)
	{
		long lFreq = pIpmDoc->GetExpressRefreshFrequency();
		if (nCheck == 1)
			pIpmCtrl->StartExpressRefresh(lFreq);
		else
			pIpmCtrl->StartExpressRefresh(0);
	}
}

void CfMainFrame::OnEditChangeFrequency()
{
	TRACE0("CfMainFrame::OnEditChangeFrequency\n");
	CString strText;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CEdit* pEdit = (CEdit*)m_wndDlgBar.GetDlgItem(IDM_REFRESH_FREQUENCY);
	pEdit->GetWindowText(strText);
	int nFrequency = _ttoi(strText);
	pIpmDoc->SetExpressRefreshFrequency(nFrequency);
	OnCheckExpresRefresh();
}

void CfMainFrame::OnUpdateResourceType(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateNullResource(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateInternalSession(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateInactiveTransaction(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateSystemLockList(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateExpresRefresh(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateEditFrequency(CCmdUI* pCmdUI)
{
	BOOL bEnable = ShouldEnable();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_EXPRESS_REFRESH))->GetCheck();
	pCmdUI->Enable(bEnable && (nCheck == 1));
}

void CfMainFrame::OnDestroy() 
{
	CFrameWnd::OnDestroy();
}

void CfMainFrame::OnUpdateExpandBranch(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateExpandAll(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateCollapseBranch(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateCollapseAll(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfMainFrame::OnUpdateShutDown(CCmdUI* pCmdUI) 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	BOOL bConnected = pIpmDoc->IsConnected();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pCmdUI->Enable(bConnected && pIpmCtrl && pIpmCtrl->IsEnabledCloseServer());
}

void CfMainFrame::OnUpdateForceRefresh(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CfMainFrame::OnFileOpen() 
{
#if defined (_NO_OPENSAVE)
	return;
#endif

	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (!pIpmCtrl)
		return;
	CString strFullName;
	CFileDialog dlg(
		TRUE,
		NULL,
		NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Ingres Visual Monitor Files (*.vdbamon)|*.vdbamon||"));

	if (dlg.DoModal() != IDOK)
		return; 

	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		AfxMessageBox (IDS_UNKNOWN_ENVIRONMENT_FILE);
		return;
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);

		if (strExt.CompareNoCase (_T("vdbamon")) != 0)
		{
			AfxMessageBox (IDS_UNKNOWN_ENVIRONMENT_FILE);
			return;
		}
	}

	IStream* pStream = NULL;
	try
	{
		short arrayFilter[10];
		short nVal;
		int nDim = 0;
		CdIpm* pDoc = (CdIpm*)GetActiveDocument();
		ASSERT(pDoc);
		if (!pDoc)
			throw;

		CaCompoundFile compoundFile(strFullName);
		//
		// Property Data:
		OpenStreamInit (pIpmCtrl->GetControlUnknown(), theApp.m_tchszIpmProperty, compoundFile.GetRootStorage());
		//
		// Container Data:
		pStream = compoundFile.OpenStream(NULL, theApp.m_tchszContainerData, STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::load);

			pDoc->Serialize(ar);
			m_lNode.Serialize(ar);
			m_lServer.Serialize(ar);
			m_lUser.Serialize(ar);
			
			ar >> nDim;
			for (int i=0; i<nDim; i++)
			{
				ar >> nVal;
				arrayFilter[i] = nVal;
			}
		}
		//
		// Ipm Data:
		pStream = compoundFile.OpenStream(NULL, theApp.m_tchszIpmData, STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			pIpmCtrl->Loading((LPUNKNOWN)(pStream));
			pStream->Release();
		}
		//
		// If the control is not created, create it but not connect.
		pDoc->UpdateAllViews(NULL, (LPARAM)3);

		FRAMEDATASTRUCT data;
		memset (&data, 0, sizeof(data));
		data.arrayFilter = arrayFilter;
		data.nDim   = nDim;
		UpdateLoadedData(pDoc, &data);

		pDoc->SetWorkingFile(strFullName);
		pDoc->UpdateAllViews(NULL, (LPARAM)5);
		return;
	}
	catch (CArchiveException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CMemoryException *e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_LOADENVIRONMENT);
	}
	if (pStream)
		pStream->Release();
}

void CfMainFrame::SaveWorkingFile(CString& strFullName)
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (!pIpmCtrl)
		return;
	IStream* pStream = NULL;
	try
	{
		CaCompoundFile compoundFile(strFullName);

		//
		// Property:
		SaveStreamInit (pIpmCtrl->GetControlUnknown(), theApp.m_tchszIpmProperty, compoundFile.GetRootStorage());
		//
		// Container Data:
		pStream = compoundFile.NewStream(NULL, theApp.m_tchszContainerData, STGM_DIRECT|STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::store);

			pIpmDoc->Serialize(ar);
			m_lNode.Serialize(ar);
			m_lServer.Serialize(ar);
			m_lUser.Serialize(ar);
			short arrayFilter[5];
			GetFilters(arrayFilter, 5);
			ar << 5;
			for (int i=0; i<5; i++)
			{
				ar << arrayFilter[i];
			}
		}
		//
		// Ipm Data:
		pStream = compoundFile.NewStream(NULL, theApp.m_tchszIpmData, STGM_DIRECT|STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE);
		if (pStream)
		{
			COleStreamFile file (pStream);
			CArchive ar(&file, CArchive::store);
		
			IStream* pStreamSql = NULL;
			pIpmCtrl->Storing((LPUNKNOWN*)(&pStreamSql));
			if (pStreamSql)
			{
				//
				// 2) Store the data of the SqlQuery control:
				ULONG lRead = 0;
				DWORD dwCount = 4090;
				BYTE buffer [4096];
				LARGE_INTEGER nDisplacement;
				nDisplacement.QuadPart = 0;
				pStreamSql->Seek(nDisplacement, STREAM_SEEK_SET, NULL);
				pStreamSql->Read((void*)buffer, dwCount, &lRead);
				while (lRead > 0)
				{
					ar.Write ((void*)buffer, lRead);
					pStreamSql->Read((void*)buffer, dwCount, &lRead);
				}

				pStreamSql->Release();
			}
		}
		return;
	}
	catch (CArchiveException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}
	catch (CMemoryException *e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_SAVEENVIRONMENT);
	}
}


void CfMainFrame::OnFileSave() 
{
#if defined (_NO_OPENSAVE)
	return;
#endif

	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (!pIpmCtrl)
		return;

	CString strFullName;
	CFileDialog dlg(
	    FALSE,
	    NULL,
	    NULL,
	    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	    _T("Ingres Visual Monitor Files (*.vdbamon)|*.vdbamon||"));

	if (dlg.DoModal() != IDOK)
		return; 
	strFullName = dlg.GetPathName ();
	int nDot = strFullName.ReverseFind(_T('.'));
	if (nDot==-1)
	{
		strFullName += _T(".vdbamon");
	}
	else
	{
		CString strExt = strFullName.Mid(nDot+1);
		if (strExt.IsEmpty())
			strFullName += _T("vdbamon");
		else
		if (strExt.CompareNoCase (_T("vdbamon")) != 0)
			strFullName += _T(".vdbamon");
	}

	SaveWorkingFile(strFullName);
}


void CfMainFrame::OnUpdateFileOpen(CCmdUI* pCmdUI) 
{
	BOOL bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (pIpmCtrl && IsWindow(pIpmCtrl->m_hWnd) && pIpmCtrl->IsWindowVisible())
		bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnViewToolbar() 
{
	// TODO: Add your command handler code here
	CMenu* pMenu = GetMenu();
	if (!pMenu)
		return;
	BOOL bShow = TRUE;
	UINT nState = pMenu->GetMenuState(ID_VIEW_TOOLBAR2, MF_BYCOMMAND);
	if (nState & MF_CHECKED)
	{
		pMenu->CheckMenuItem(ID_VIEW_TOOLBAR2, MF_UNCHECKED|MF_BYCOMMAND);
		bShow = FALSE;
	}
	else
	{
		pMenu->CheckMenuItem(ID_VIEW_TOOLBAR2, MF_CHECKED|MF_BYCOMMAND);
		bShow = TRUE;
	}

	ShowControlBar(&m_wndReBar, bShow, TRUE);
}

void CfMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	if (IsWindow (m_wndReBar.m_hWnd))
	{
		CReBarCtrl& bctrl = m_wndReBar.GetReBarCtrl();
		bctrl.MaximizeBand(1);
		m_wndReBar.Invalidate();
	}
}

void CfMainFrame::OnPreference() 
{
	USES_CONVERSION;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (pIpmCtrl && IsWindow(pIpmCtrl->m_hWnd))
	{
		CString strSqlSettingCaption;
		strSqlSettingCaption.LoadString(IDS_MAINTOOLBAR_TITLE);
		LPCOLESTR lpCaption = T2COLE((LPTSTR)(LPCTSTR)strSqlSettingCaption);

		IUnknown* pUk = pIpmCtrl->GetControlUnknown();
		ISpecifyPropertyPagesPtr pSpecifyPropertyPages = pUk;
		CAUUID pages;
		HRESULT hr = pSpecifyPropertyPages->GetPages( &pages );
		if(FAILED(hr ))
			return;

		m_bPropertyChange = FALSE;
		hr = OleCreatePropertyFrame(
			m_hWnd,
			10,
			10,
			lpCaption,
			1,
			(IUnknown**) &pUk,
			pages.cElems,
			pages.pElems,
			0, 
			0L,
			NULL );

		CoTaskMemFree( (void*) pages.pElems );
		//
		// Save the properties to file IIGUIS.PRF:
		BOOL bSave2Default = FALSE;
		CMenu* pMenu = GetMenu();
		if (pMenu)
		{
			UINT nState = pMenu->GetMenuState(IDM_PREFERENCE_SAVE, MF_BYCOMMAND);
			if (nState == MF_CHECKED)
				bSave2Default = TRUE;
		}
		if (m_bPropertyChange && bSave2Default)
		{
			BOOL bOk = SaveStreamInit(pIpmCtrl->GetControlUnknown(), _T("Ipm"));
			if (!bOk)
				AfxMessageBox (IDS_FAILED_2_SAVESTORAGE);
		}
	}
}

void CfMainFrame::OnUpdatePreference(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (pIpmCtrl && IsWindow(pIpmCtrl->m_hWnd))
		bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}

void CfMainFrame::OnUpdatePreferenceSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
}

void CfMainFrame::OnPreferenceSave() 
{
	CMenu* pMenu = GetMenu();
	UINT nState = pMenu->GetMenuState(IDM_PREFERENCE_SAVE, MF_BYCOMMAND);
	if (nState == MF_CHECKED)
	{
		pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_UNCHECKED);
		m_bSavePreferenceAsDefault = FALSE;
	}
	else
	{
		pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_CHECKED);
		m_bSavePreferenceAsDefault = TRUE;
	}
}

void CfMainFrame::OnClose() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	CString strWorkingFile = pIpmDoc->GetWorkingFile();
	if (!strWorkingFile.IsEmpty())
	{
		int nAnsver = AfxMessageBox (IDS_MSG_ASKFORSAVING_WORKINGFILE, MB_ICONQUESTION|MB_YESNO);
		if (nAnsver == IDYES)
		{
			SaveWorkingFile(strWorkingFile);
		}
	}

	PermanentState(FALSE);
	CFrameWnd::OnClose();
}

void CfMainFrame::PermanentState(BOOL bLoad)
{
#if defined (_PERSISTANT_STATE_)
	CaPersistentStreamState file(_T("Vdbamon"), bLoad);
	if (!file.IsStateOK())
		return;
	if (bLoad)
	{
		CArchive ar(&file, CArchive::load);
		ar >> m_bSavePreferenceAsDefault;

		CMenu* pMenu = GetMenu();
		if (m_bSavePreferenceAsDefault)
			pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_CHECKED);
		else
			pMenu->CheckMenuItem(IDM_PREFERENCE_SAVE, MF_BYCOMMAND|MF_UNCHECKED);
	}
	else
	{
		CArchive ar(&file, CArchive::store);
		ar << m_bSavePreferenceAsDefault;
	}
#endif
}


#define MASK_SETNODE     1
#define MASK_SETSERVER   2
#define MASK_SETUSER     4
void CfMainFrame::HandleCommandLine(CdIpm* pDoc)
{
	BOOL bNoApply = FALSE;
	BOOL bMinToolbar = FALSE;
	UINT nSet = 0;
	BOOL bEnableControl = FALSE;
	BOOL b2Connect = FALSE;
	BOOL bError = FALSE;
	CTypedPtrList <CObList, CaKeyValueArg*>& listKey = theApp.m_cmdLineArg.GetKeys();
	POSITION pos = listKey.GetHeadPosition();
	while (pos != NULL)
	{
		CaKeyValueArg* pKey = listKey.GetNext (pos);
		if (pKey && pKey->IsMatched())
		{
			CString strValue;
			CString strKey = pKey->GetKey();
			if (strKey.CompareNoCase(_T("-node")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					m_pComboNode = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
					if (m_pComboNode)
					{
						int nInx = m_pComboNode->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetNode(strValue);
							m_pComboNode->SetCurSel(nInx);
							nSet |= MASK_SETNODE;
						}
						else
						{
							bError = TRUE;
							break; 
						}
					}
				}
			}
			else
			if (strKey.CompareNoCase(_T("-server")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					if (!QueryServer())
					{
						bError = TRUE;
						break; 
					}
					m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
					if (m_pComboServer)
					{
						int nInx = m_pComboServer->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetServer(strValue);
							m_pComboServer->SetCurSel(nInx);
							nSet |= MASK_SETSERVER;
						}
					}
				}
			}
			else
			if (strKey.CompareNoCase(_T("-u")) == 0)
			{
				b2Connect = TRUE;
				strValue = pKey->GetValue();
				if (!strValue.IsEmpty())
				{
					if (!QueryUser())
					{
						bError = TRUE;
						break; 
					}
					m_pComboUser = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
					if (m_pComboUser)
					{
						int nInx = m_pComboUser->FindStringExact(-1, strValue);
						if (nInx != CB_ERR)
						{
							pDoc->SetUser(strValue);
							m_pComboUser->SetCurSel(nInx);
							nSet |= MASK_SETUSER;
						}
						m_pComboUser->EnableWindow(bEnableControl);
					}
				}
			}
			else
			if (strKey.CompareNoCase(_T("-maxapp")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					ShowWindow(SW_SHOWMAXIMIZED);
				}
			}
			else
			if (strKey.CompareNoCase(_T("-noapply")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					bNoApply = TRUE;
				}
			}
#if defined (_MIN_TOOLBARS_)
			else
			if (strKey.CompareNoCase(_T("-mintoolbar")) == 0)
			{
				strValue = pKey->GetValue();
				if (strValue.CompareNoCase(_T("TRUE")) == 0)
				{
					bMinToolbar = TRUE;
				}
			}
#endif
		}
	}

	if (b2Connect)
	{
		if (bNoApply)
		{
			//
			// If the apply button is not allowed then disable all the combo (node, server, user):
			nSet = MASK_SETNODE|MASK_SETUSER|MASK_SETSERVER;
		}

		m_bCommandLine = TRUE;
		m_pComboNode = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_NODE);
		m_pComboServer = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_SERVER_CLASS);
		m_pComboUser = (CComboBoxEx*)m_wndDlgConnectBar.GetDlgItem(IDM_USER);
		if (m_pComboNode && m_pComboServer && m_pComboUser)
		{
			if (nSet & MASK_SETNODE)
				m_pComboNode->EnableWindow(bEnableControl);
			if (nSet & MASK_SETSERVER)
			{
				m_pComboNode->EnableWindow(bEnableControl);
				m_pComboServer->EnableWindow(bEnableControl);
			}
			if (nSet & MASK_SETUSER)
			{
				m_pComboNode->EnableWindow(bEnableControl);
				m_pComboServer->EnableWindow(bEnableControl);
				m_pComboUser->EnableWindow(bEnableControl);
			}

			if (!bError)
				pDoc->UpdateAllViews(NULL, 2);
		}

		if (bMinToolbar)
		{
			CReBarCtrl& rb = m_wndReBar.GetReBarCtrl();
			rb.DeleteBand(0);
		}
		m_bActivateApply = !bNoApply || !((nSet & MASK_SETNODE) && (nSet & MASK_SETSERVER) && (nSet & MASK_SETUSER));
	}
}

void CfMainFrame::OnUpdateGo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_bActivateApply);
}

void CfMainFrame::OnHistoricSqlError() 
{
	CdIpm* pDoc = (CdIpm*)GetActiveDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;

	CDlgSqlErrorHistory dlg;
	dlg.m_FilesErrorClass = pDoc->GetClassFilesErrors();
	if (!dlg.m_FilesErrorClass->GetStoreInFilesEnable())
		return; //Files names initializations failed, nothing to display
	dlg.DoModal();
	
}

void CfMainFrame::OnUpdateHistoricSqlError(CCmdUI* pCmdUI) 
{
	BOOL bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}

BOOL CfMainFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	TRACE0("CfMainFrame::OnHelpInfo\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	ASSERT(pIpmDoc);
	if (!pIpmDoc)
		return FALSE;
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (pIpmCtrl && IsWindow (pIpmCtrl->m_hWnd) && pIpmCtrl->IsWindowVisible())
	{
		pIpmCtrl->SendMessageToDescendants(WM_HELP);
		return TRUE;
	}
	OnDefaultHelp();
	return TRUE;
}

void CfMainFrame::OnDefaultHelp()
{
	APP_HelpInfo(m_hWnd, 0);
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#if defined(MAINWIN)
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += _T("vdbamon.chm");
	}
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		::HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		::HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);
	return TRUE;
}
