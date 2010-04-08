/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmaxfrm.cpp : implementation file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the Frame CfIpm class  (MFC frame/view/doc).
**
** History:
**
** 20-Fev-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate ipm.ocx.
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "ipmaxfrm.h"
#include "ipmaxdoc.h"
#include "cmdline.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern "C"
{
#include "monitor.h" // LPSTYPELOCK
#include "main.h"    // TYPEDOC_MONITOR
#include "dbadlg1.h"    // FIND_MATCHCASE
int OneWinExactlyGetWinType();
int SetOIVers (int oivers);
};

static void InitializeComboResourceType(CComboBox* pCombo)
{
	// insert in combobox scanning Type_Lock array from monitor.cpp.
	// the combobox will sort items
	LPSTYPELOCK pTypeLock;
	for (pTypeLock = Type_Lock; pTypeLock->iResource != -1; pTypeLock++) 
	{
		int restype = pTypeLock->iResource;
		if (restype != RESTYPE_ALL && restype != RESTYPE_DATABASE && restype != RESTYPE_TABLE && restype != RESTYPE_PAGE)
		{
			int index = pCombo->AddString ((LPCTSTR)pTypeLock->szStringLock);
			pCombo->SetItemData(index, restype);
		}
	}
	//
	// force insert of "All Other resource types" at first position
	CString strType;
	strType.LoadString (IDS_ALLRESOURCETYPES);
	int index = pCombo->InsertString (0, (LPCTSTR)strType);
	pCombo->SetItemData(index, (DWORD)RESTYPE_ALL);
}


IMPLEMENT_DYNCREATE(CfIpm, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CfIpm, CMDIChildWnd)
	//{{AFX_MSG_MAP(CfIpm)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MDIACTIVATE()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_BUTTON_FIND, OnUpdateButtonFind)

	// 
	// Dialogbar notification:
	// **********************
	// Resource Type Combo notification:
	ON_CBN_SELCHANGE(IDM_IPMBAR_ALLRESOURCES,            OnSelChangeResourceType)
	ON_BN_CLICKED   (IDM_IPMBAR_NULLRESOURCE,            OnCheckNullResource)
	ON_BN_CLICKED   (IDM_IPMBAR_INTERNALSESSION,         OnCheckInternalSession)
	ON_BN_CLICKED   (IDM_IPMBAR_INACTIVETRANS,           OnCheckInactiveTransaction)
	ON_BN_CLICKED   (IDM_IPMBAR_SYSLOCKLISTS,            OnCheckSystemLockList)
	ON_BN_CLICKED   (IDM_IPMBAR_EXPRESS_REFRESH,         OnCheckExpresRefresh)
	ON_EN_CHANGE    (IDM_IPMBAR_REFRESH_FREQUENCY,       OnEditChangeFrequency)
	ON_EN_KILLFOCUS (IDM_IPMBAR_REFRESH_FREQUENCY,       OnEditKillfocusFrequency)
	ON_COMMAND      (IDM_IPMBAR_REFRESH,                 OnForceRefresh)
	ON_COMMAND      (IDM_IPMBAR_SHUTDOWN,                OnShutDown)
	ON_COMMAND      (ID_BUTTON_CLASSB_EXPANDBRANCH,      OnExpandBranch)
	ON_COMMAND      (ID_BUTTON_CLASSB_EXPANDALL,         OnExpandAll)
	ON_COMMAND      (ID_BUTTON_CLASSB_COLLAPSEBRANCH,    OnCollapseBranch)
	ON_COMMAND      (ID_BUTTON_CLASSB_COLLAPSEALL,       OnCollapseAll)

	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_ALLRESOURCES,        OnUpdateResourceType)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_NULLRESOURCE,        OnUpdateNullResource)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_INTERNALSESSION,     OnUpdateInternalSession)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_INACTIVETRANS,       OnUpdateInactiveTransaction)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_SYSLOCKLISTS,        OnUpdateSystemLockList)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_EXPRESS_REFRESH,     OnUpdateExpresRefresh)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_REFRESH_FREQUENCY,   OnUpdateEditFrequency)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_REFRESH,             OnUpdateForceRefresh)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_SHUTDOWN,            OnUpdateShutDown)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_EXPANDBRANCH,  OnUpdateExpandBranch)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_EXPANDALL,     OnUpdateExpandAll)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_COLLAPSEBRANCH,OnUpdateCollapseBranch)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_COLLAPSEALL,   OnUpdateCollapseAll)
	ON_MESSAGE (WM_USER_GETMFCDOCTYPE, OnGetDocumentType)
	ON_MESSAGE(WM_USERMSG_FIND, OnFind)
END_MESSAGE_MAP()


LONG CfIpm::OnGetDocumentType(WPARAM wParam, LPARAM lParam)
{
	CdIpm* pDoc = (CdIpm*)GetIpmDoc();
	if (pDoc)
		return TYPEDOC_MONITOR;
	else
		return TYPEDOC_UNKNOWN;
}


/////////////////////////////////////////////////////////////////////////////
// CfIpm construction/destruction

CfIpm::CfIpm()
{
	m_bAllCreated = FALSE;
}

CfIpm::~CfIpm()
{
}


void CfIpm::GetFilter(CaIpmFrameFilter* pFrameData)
{
	//
	// Update Filters:
	ASSERT(pFrameData);
	if (!pFrameData)
		return;
	CArray <short, short>& filter = pFrameData->GetFilter();
	filter[0] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_NULLRESOURCE))->GetCheck();
	filter[1] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INTERNALSESSION))->GetCheck();
	filter[2] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_SYSLOCKLISTS))->GetCheck();
	filter[3] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INACTIVETRANS))->GetCheck();
	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_ALLRESOURCES);
	int nSel = pCombo->GetCurSel();
	filter[4] = (short)pCombo->GetItemData(nSel);
	filter[5] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_EXPRESS_REFRESH_ONLYIF))->GetCheck();

	int nCheckExpressRefresh = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_EXPRESS_REFRESH))->GetCheck();
	pFrameData->SetActivatedExpressRefresh((BOOL)nCheckExpressRefresh);
}


void CfIpm::UpdateLoadedData(CdIpm* pDoc)
{
	CaIpmFrameFilter& filter = pDoc->GetFilter();
	CArray <short, short>& filterValue = filter.GetFilter();
	//
	// Update Filters:
	ASSERT(filterValue.GetSize() == 6);
	if (filterValue.GetSize() != 6)
		return;
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_NULLRESOURCE))->SetCheck(filterValue[0]);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INTERNALSESSION))->SetCheck(filterValue[1]);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_SYSLOCKLISTS))->SetCheck(filterValue[2]);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INACTIVETRANS))->SetCheck(filterValue[3]);

	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_ALLRESOURCES);
	int i, nCount = pCombo->GetCount();
	for (i=0; i<nCount; i++)
	{
		if (filterValue[4] == (short)pCombo->GetItemData(i))
		{
			pCombo->SetCurSel(i);
			break;
		}
	}
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_EXPRESS_REFRESH_ONLYIF))->SetCheck(filterValue[5]);

	//
	// Update Express Refresh Frequency:
	CString strFrequency;
	strFrequency.Format(_T("%d"), pDoc->GetExpressRefreshFrequency());
	((CEdit*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_REFRESH_FREQUENCY))->SetWindowText (strFrequency);
	((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_EXPRESS_REFRESH))->SetCheck(filter.IsActivatedExpressRefresh());
	OnCheckExpresRefresh();
}


void CfIpm::ShowIpmControl(BOOL bShow)
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

int CfIpm::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	//
	// The application is running in the context driven mode:
	BOOL bContextDriven = (OneWinExactlyGetWinType() == TYPEDOC_MONITOR);
	m_wndDlgBar.SetContextDriven(bContextDriven);

	EnableDocking(CBRS_ALIGN_ANY);
	UINT nStyle = WS_CHILD|WS_THICKFRAME|CBRS_SIZE_DYNAMIC|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_HIDE_INPLACE;
	if (!m_wndDlgBar.Create(this, IDD_IPMBAR, nStyle, 5000))
	{
		TRACE0("CfIpm::OnCreate: failed to create dialog bar.\n");
		return -1; // fail to create
	}
	CString str;
	str.LoadString (IDS_IPMBAR_TITLE);
	m_wndDlgBar.SetWindowText (str);
	m_wndDlgBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT|CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
	DockControlBar(&m_wndDlgBar);
	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem (IDM_IPMBAR_ALLRESOURCES);

	InitializeComboResourceType(pCombo);
	pCombo->SetCurSel (0);
	m_bAllCreated = TRUE;

	//
	// Set the default Express Refresh Frequency:
	if (!m_pIpmDoc)
		m_pIpmDoc = (CdIpm*)GetIpmDoc();
	CString strFrequency;
	strFrequency.Format(_T("%d"), m_pIpmDoc->GetExpressRefreshFrequency());
	((CEdit*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_REFRESH_FREQUENCY))->SetWindowText (strFrequency);

	if (m_pIpmDoc && m_pIpmDoc->IsLoadedDoc())
	{
		UpdateLoadedData(m_pIpmDoc);
		DisableExpresRefresh();
	}

	return 0;
}

BOOL CfIpm::PreCreateWindow(CREATESTRUCT& cs)
{
	if (IsUnicenterDriven()) 
	{
		CuCmdLineParse* pCmdLineParse = GetAutomatedGeneralDescriptor();
		ASSERT (pCmdLineParse);
		if (pCmdLineParse->DoWeMaximizeWin())
		{
			//
			// Note: Maximize MUST be combined with VISIBLE
			cs.style |= WS_MAXIMIZE | WS_VISIBLE;
			if (!theApp.CanCloseContextDrivenFrame())
			{
				cs.style &=~WS_SYSMENU;
			}
		}
	}
	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CfIpm diagnostics

#ifdef _DEBUG
void CfIpm::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CfIpm::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

void CfIpm::GetFilters(short* arrayFilter, int nSize)
{
	ASSERT(nSize >= 5);
	for (int i=0; i<nSize; i++)
		arrayFilter[i] = 0;
	arrayFilter[0] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_NULLRESOURCE))->GetCheck();
	arrayFilter[1] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INTERNALSESSION))->GetCheck();
	arrayFilter[2] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_SYSLOCKLISTS))->GetCheck();
	arrayFilter[3] = ((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INACTIVETRANS))->GetCheck();
	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_ALLRESOURCES);
	int nSel = pCombo->GetCurSel();
	arrayFilter[4] = (short)pCombo->GetItemData(nSel);
}


BOOL CfIpm::ShouldEnable()
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	ASSERT(pIpmDoc);
	if (!pIpmDoc)
		return FALSE;
	BOOL bConnected = pIpmDoc->IsConnected();
	return bConnected;
}

void CfIpm::DisableExpresRefresh()
{
	if (!m_bAllCreated)
		return;
	TRACE0("CfIpm::DisableExpresRefresh\n");
	CButton* pButton = (CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_EXPRESS_REFRESH);
	short nCheck = (short)pButton->GetCheck();
	if (nCheck==1)
	{
		pButton->SetCheck(0);
		OnCheckExpresRefresh();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CfIpm message handlers
void CfIpm::OnForceRefresh() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (pIpmCtrl->IsWindowVisible())
	{
		pIpmCtrl->ForceRefresh();
	}
}

void CfIpm::OnShutDown() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->ItemShutdown();
}

void CfIpm::OnExpandBranch() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->ExpandBranch();
	
}

void CfIpm::OnExpandAll() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->ExpandAll();
}

void CfIpm::OnCollapseBranch() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->CollapseBranch();
}

void CfIpm::OnCollapseAll() 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pIpmCtrl->CollapseAll();
}


void CfIpm::OnSelChangeResourceType()
{
	TRACE0("CfIpm::OnSelChangeResourceType\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	CComboBox* pCombo = (CComboBox*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_ALLRESOURCES);
	int nSel = pCombo->GetCurSel();
	short nResType = (short)pCombo->GetItemData(nSel);
	pIpmCtrl->ResourceTypeChange(nResType);
}

void CfIpm::OnCheckNullResource()
{
	TRACE0("CfIpm::OnCheckNullResource\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_NULLRESOURCE))->GetCheck();
	pIpmCtrl->NullResource(nCheck);
}

void CfIpm::OnCheckInternalSession()
{
	TRACE0("CfIpm::OnCheckInternalSession\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INTERNALSESSION))->GetCheck();
	pIpmCtrl->InternalSession(nCheck);
}

void CfIpm::OnCheckInactiveTransaction()
{
	TRACE0("CfIpm::OnCheckInactiveTransaction\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_INACTIVETRANS))->GetCheck();
	pIpmCtrl->InactiveTransaction(nCheck);
}

void CfIpm::OnCheckSystemLockList()
{
	TRACE0("CfIpm::OnCheckSystemLockList\n");
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_SYSLOCKLISTS))->GetCheck();
	pIpmCtrl->SystemLockList(nCheck);
}

void CfIpm::OnCheckExpresRefresh()
{
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_EXPRESS_REFRESH))->GetCheck();
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

void CfIpm::OnEditChangeFrequency()
{
	TRACE0("CfIpm::OnEditChangeFrequency\n");
	CString strText;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	CEdit* pEdit = (CEdit*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_REFRESH_FREQUENCY);
	pEdit->GetWindowText(strText);
	int nFrequency = _ttoi(strText);
	pIpmDoc->SetExpressRefreshFrequency(nFrequency);
	OnCheckExpresRefresh();
}

void CfIpm::OnEditKillfocusFrequency()
{
	TRACE0("CfIpm::OnEditKillfocusFrequency\n");
	CString strText;
	CEdit* pEdit = (CEdit*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_REFRESH_FREQUENCY);
	pEdit->GetWindowText(strText);
	int nFrequency = _ttoi(strText);
	if (nFrequency < 2)
		pEdit->SetWindowText(_T("2"));
}

void CfIpm::OnUpdateResourceType(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateNullResource(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateInternalSession(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateInactiveTransaction(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateSystemLockList(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateExpresRefresh(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateEditFrequency(CCmdUI* pCmdUI)
{
	BOOL bEnable = ShouldEnable();
	short nCheck = (short)((CButton*)m_wndDlgBar.GetDlgItem(IDM_IPMBAR_EXPRESS_REFRESH))->GetCheck();
	pCmdUI->Enable(bEnable && (nCheck == 1));
}

void CfIpm::OnDestroy() 
{
	theApp.SetLastDocMaximizedState(IsZoomed());
	CMDIChildWnd::OnDestroy();
}

void CfIpm::OnUpdateExpandBranch(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateExpandAll(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateCollapseBranch(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateCollapseAll(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ShouldEnable());
}

void CfIpm::OnUpdateShutDown(CCmdUI* pCmdUI) 
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	BOOL bConnected = pIpmDoc->IsConnected();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	pCmdUI->Enable(bConnected && pIpmCtrl && pIpmCtrl->IsEnabledCloseServer());
}

void CfIpm::OnUpdateForceRefresh(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CfIpm::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
}


void CfIpm::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

	if (bActivate) 
	{
		ASSERT (pActivateWnd == this);
		CdIpm* pDoc = (CdIpm*)GetIpmDoc();
		ASSERT(pDoc);

		if (pDoc)
		{
			int iVers = pDoc->GetIngresVersion();
			if (iVers != -1)
				SetOIVers(iVers);
		}
	}
}

void CfIpm::OnUpdateButtonFind(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

LONG CfIpm::OnFind(WPARAM wParam, LPARAM lParam)
{
	LPCTSTR lpszWhat = (LPCTSTR)lParam;
	long nMode = 0;
	if (wParam & FIND_MATCHCASE)
		nMode |= FR_MATCHCASE;
	if (wParam & FIND_WHOLEWORD)
		nMode |= FR_WHOLEWORD;
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	BOOL bConnected = pIpmDoc->IsConnected();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (bConnected && pIpmCtrl)
	{
		CString strMsg = _T("");
		short sResult = pIpmCtrl->FindAndSelectTreeItem(lpszWhat, nMode);
		switch (sResult)
		{
		case 0:
			strMsg.Format(IDS_FIND_TREE_NOTFOUND, lpszWhat);
			break;
		case 1:
			strMsg.Format(_T("No Other Occurrences of %s"), lpszWhat);
			break;
		}
		if (!strMsg.IsEmpty())
		{
			AfxMessageBox(strMsg, MB_ICONASTERISK);
		}
	}

	return 0;
}

long CfIpm::GetHelpID()
{
	CdIpm* pIpmDoc = (CdIpm*)GetIpmDoc();
	if (!pIpmDoc)
		return 0;
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (!pIpmCtrl)
		return 0;
	return pIpmCtrl->GetHelpID();
}

extern "C" DWORD GetMonitorDocHelpId(HWND hWnd)
{
	CWnd *pWnd;
	pWnd = CWnd::FromHandlePermanent(hWnd);
	ASSERT (pWnd);
	if (!pWnd)
		return HELPID_MONITOR;  // default help screen
	CfIpm* pIpmFrm = (CfIpm *)pWnd;
	long nHelpID = pIpmFrm->GetHelpID();
	return (nHelpID == 0)? (DWORD)HELPID_MONITOR: (DWORD)nHelpID;
}

// from IpmFrame.cpp
extern "C" int MfcGetNumberOfIpmObjects(HWND hwndDoc)
{
	CWnd *pWnd = CWnd::FromHandlePermanent(hwndDoc);
	ASSERT (pWnd);
	if (!pWnd)
		return -1;
	CfIpm* pIpmFrame = (CfIpm *)pWnd;
	CdIpm* pIpmDoc = (CdIpm*)pIpmFrame->GetIpmDoc();
	ASSERT (pIpmDoc);
	if (!pIpmDoc)
		return -1;
	BOOL bConnected = pIpmDoc->IsConnected();
	CuIpm* pIpmCtrl = pIpmDoc->GetIpmCtrl();
	if (bConnected && pIpmCtrl)
	{
		return pIpmCtrl->GetMonitorObjectsCount();
	}
	return -1;
}
