/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdpenle.cpp, Implementation file
**    Project  : INGRES II/ VDBA (Installation Level Auditing).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control. The DOM PROP Enabled Level Page
**
** 
** Histoty:
** xx-Oct-1998 (uk$so01)
**    Created.
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgdpenle.h"
#include "sqlselec.h"
#include "vtree.h"

extern BOOL INGRESII_QuerySecurityState (CTypedPtrList<CObList, CaAuditEnabledLevel*>& listEnableLevel);
extern BOOL INGRESII_UpdateSecurityState(CTypedPtrList<CObList, CaAuditEnabledLevel*>& listEnableLevel);

extern "C"
{
#include "dbaginfo.h"
#include "domdata.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CaInstallLevelSecurityState::CaInstallLevelSecurityState(int nNodeHandle)
{
	TCHAR  tchszConnection [256];
	LPTSTR lpszVnodeName = (LPTSTR)GetVirtNodeName (nNodeHandle);
	m_nNodeHandle = nNodeHandle;
	m_nSession  = -1;
	ASSERT (lpszVnodeName);
	if (!lpszVnodeName)
		return;
	wsprintf (tchszConnection,_T("%s::%s"), lpszVnodeName, _T("iidbdb"));
	int ires = Getsession ((LPUCHAR)tchszConnection, SESSION_TYPE_CACHENOREADLOCK, &m_nSession);
	if (ires != RES_SUCCESS && m_nSession != -1)
	{
		ires = ReleaseSession(m_nSession, RELEASE_COMMIT);
		m_nSession = -1;
	}
}

CaInstallLevelSecurityState::~CaInstallLevelSecurityState()
{
	if (m_nSession != -1)
		ReleaseSession(m_nSession, RELEASE_COMMIT);
}

BOOL CaInstallLevelSecurityState::QueryState (CTypedPtrList<CObList, CaAuditEnabledLevel*>& listEnableLevel)
{
	ASSERT (m_nSession != -1);
	if (m_nSession == -1)
		return FALSE;
	ActivateSession (m_nSession);
	return INGRESII_QuerySecurityState (listEnableLevel);
}

BOOL CaInstallLevelSecurityState::UpdateState(CTypedPtrList<CObList, CaAuditEnabledLevel*>& listEnableLevel)
{
	ASSERT (m_nSession != -1);
	if (m_nSession == -1)
		return FALSE;
	ActivateSession (m_nSession);
	return INGRESII_UpdateSecurityState(listEnableLevel);
}



CuDlgDomPropInstallationLevelEnabledLevel::CuDlgDomPropInstallationLevelEnabledLevel(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropInstallationLevelEnabledLevel::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropInstallationLevelEnabledLevel)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgDomPropInstallationLevelEnabledLevel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropInstallationLevelEnabledLevel)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropInstallationLevelEnabledLevel, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropInstallationLevelEnabledLevel)
	ON_WM_SIZE()
	ON_NOTIFY(NM_DBLCLK, IDC_MFC_LIST1, OnDblclkMfcList1)
	ON_NOTIFY(LVN_DELETEITEM, IDC_MFC_LIST1, OnDeleteitemMfcList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE, OnQueryType)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropInstallationLevelEnabledLevel message handlers

void CuDlgDomPropInstallationLevelEnabledLevel::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropInstallationLevelEnabledLevel::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
	m_ImageCheck.Create (IDB_CHECK_NOFRAME, 16, 1, RGB (255, 0, 0));
	m_cListCtrl.SetCheckImageList(&m_ImageCheck);
	m_ImageList.Create(IDB_BITMAP_DEFAULT, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	//
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS lsp[2] =
	{
		{_T(""),    150,  LVCFMT_LEFT,  FALSE},
		{_T(""),    100,  LVCFMT_CENTER, TRUE}
	};

	lstrcpy(lsp[0].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_AUDIT_FLAG));//_T("Audit Flag")
	lstrcpy(lsp[1].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_ENABLED));//_T("Enabled")

	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<2; i++)
	{
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = lsp[i].m_tchszHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn, (i==1)? TRUE: FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropInstallationLevelEnabledLevel::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}


LONG CuDlgDomPropInstallationLevelEnabledLevel::OnQueryType(WPARAM wParam, LPARAM lParam)
{
	ASSERT (wParam == 0);
	ASSERT (lParam == 0);
	return DOMPAGE_SPECIAL;
}


LONG CuDlgDomPropInstallationLevelEnabledLevel::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int nNodeHandle = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
		break;
	case FILTER_SETTING_CHANGE:
		VDBA_OnGeneralSettingChange(&m_cListCtrl);
		return 0;
	default:
		break;
	}

	try
	{
		ResetDisplay();

		CTypedPtrList<CObList, CaAuditEnabledLevel*> listEnableLevel;
		CaInstallLevelSecurityState securityState(nNodeHandle);
		if (!securityState.QueryState (listEnableLevel))
			return 0;
		while (!listEnableLevel.IsEmpty())
		{
			CaAuditEnabledLevel* pData = listEnableLevel.RemoveHead();
			AddItem (_T(""), pData);
		}
		DisplayItems();
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeSQLException e)
	{
		AfxMessageBox (e.m_strReason);
	}
	catch (...)
	{
		CString strMsg = _T("Unknown error occured in: CuDlgDomPropInstallationLevelEnabledLevel::OnUpdateData\n");
		TRACE0 (strMsg);
	}
	return 0L;
}

LONG CuDlgDomPropInstallationLevelEnabledLevel::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuDomPropDataPageInstallationLevelEnabledLevel")) == 0);
	CTypedPtrList<CObList, CaAuditEnabledLevel*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CaAuditEnabledLevel*>*)lParam;

	CaAuditEnabledLevel* pObj = NULL;
	try
	{
		ResetDisplay();
		while (!pListTuple->IsEmpty())
		{
			pObj = (CaAuditEnabledLevel*)pListTuple->RemoveHead();
			ASSERT (pObj);
			if (pObj)
				AddItem (_T(""), pObj);
		}
		DisplayItems();
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgDomPropInstallationLevelEnabledLevel::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaAuditEnabledLevel* p = NULL;
	CuDomPropDataPageInstallationLevelEnabledLevel* pData = NULL;
	try
	{
		pData = new CuDomPropDataPageInstallationLevelEnabledLevel ();
		CTypedPtrList<CObList, CaAuditEnabledLevel*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CaAuditEnabledLevel* pItem = (CaAuditEnabledLevel*)m_cListCtrl.GetItemData(i);
			if (!pItem)
				continue;
			p = new CaAuditEnabledLevel (pItem->m_strAuditFlag, pItem->m_bEnabled);
			listTuple.AddTail (p);
		}
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgDomPropInstallationLevelEnabledLevel::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}

void CuDlgDomPropInstallationLevelEnabledLevel::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CaAuditEnabledLevel* p = (CaAuditEnabledLevel*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (p != NULL)
		delete p;
	*pResult = 0;
}

void CuDlgDomPropInstallationLevelEnabledLevel::AddItem (LPCTSTR lpszItem, CaAuditEnabledLevel* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CuDlgDomPropInstallationLevelEnabledLevel::DisplayItems()
{
	CaAuditEnabledLevel* p = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		p = (CaAuditEnabledLevel*)m_cListCtrl.GetItemData (i);
		if (!p)
			continue;
		m_cListCtrl.SetItemText (i, 0, p->m_strAuditFlag);
		m_cListCtrl.SetCheck    (i, 1, p->m_bEnabled);
		m_cListCtrl.EnableCheck (i, 1, FALSE);
	}
}

void CuDlgDomPropInstallationLevelEnabledLevel::ResetDisplay()
{
	UpdateData (FALSE);   // Mandatory!
	m_cListCtrl.DeleteAllItems();
	VDBA_OnGeneralSettingChange(&m_cListCtrl);
	// Force immediate update of all children controls
	RedrawAllChildWindows(m_hWnd);
}
