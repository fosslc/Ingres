/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdpauus.cpp, Implementation file
**    Project  : INGRES II/ VDBA (Installation Level Auditing).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control. The DOM Property, Audit all Users Page
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
#include "dgdpauus.h"
#include "domseri.h"
#include "vtree.h"

extern "C"
{
#include "dbaginfo.h"
#include "resource.h"
#include "domdata.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgDomPropInstallationLevelAuditAllUsers::CuDlgDomPropInstallationLevelAuditAllUsers(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropInstallationLevelAuditAllUsers::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgDomPropInstallationLevelAuditAllUsers)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgDomPropInstallationLevelAuditAllUsers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropInstallationLevelAuditAllUsers)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropInstallationLevelAuditAllUsers, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropInstallationLevelAuditAllUsers)
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
// CuDlgDomPropInstallationLevelAuditAllUsers message handlers

void CuDlgDomPropInstallationLevelAuditAllUsers::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgDomPropInstallationLevelAuditAllUsers::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_MFC_LIST1, this));
	m_ImageList.Create(16, 16, TRUE, 1, 1);
	m_ImageList.AddIcon(IDI_TREE_STATIC_USER);
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	//
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS lsp[1] =
	{
		{_T(""),           200,  LVCFMT_LEFT, FALSE},
	};
	lstrcpy(lsp[0].m_tchszHeader,VDBA_MfcResourceString(IDS_TC_USER));//_T("User")
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<1; i++)
	{
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = lsp[i].m_tchszHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn); 
	}

	return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropInstallationLevelAuditAllUsers::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgDomPropInstallationLevelAuditAllUsers::OnQueryType(WPARAM wParam, LPARAM lParam)
{
	ASSERT (wParam == 0);
	ASSERT (lParam == 0);
	return DOMPAGE_LIST;
}


LONG CuDlgDomPropInstallationLevelAuditAllUsers::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int ires, nNodeHandle = (int)wParam;
	CuNameOnly* pData = NULL;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
		break;
	case FILTER_SETTING_CHANGE:
		VDBA_OnGeneralSettingChange(&m_cListCtrl);
		return 0L;
	default:
		break;
	}

	try
	{
		LPUCHAR aparentsTemp[MAXPLEVEL];
		TCHAR   buf[MAXOBJECTNAME];
		TCHAR   bufOwner[MAXOBJECTNAME];
		TCHAR   bufComplim[MAXOBJECTNAME];

		aparentsTemp[0] = aparentsTemp[1] = aparentsTemp[2] = NULL;
		memset (&bufComplim, '\0', sizeof(bufComplim));
		memset (&bufOwner, '\0', sizeof(bufOwner));

		ResetDisplay();
		ires =  DOMGetFirstObject(
			nNodeHandle,
			OT_USER,
			0,                            // level,
			aparentsTemp,                 // Temp mandatory!
			pUps->pSFilter->bWithSystem,  // bwithsystem
			NULL,                         // lpowner
			(LPUCHAR)buf,
			(LPUCHAR)bufOwner,
			(LPUCHAR)bufComplim);

		while (ires == RES_SUCCESS) 
		{
			if (bufComplim[0])
			{
				pData = new CuNameOnly(buf, FALSE);
				AddItem (_T(""), pData);
			}
			ires = DOMGetNextObject((LPUCHAR)buf, (LPUCHAR)bufOwner, (LPUCHAR)bufComplim);
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

LONG CuDlgDomPropInstallationLevelAuditAllUsers::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuDomPropDataPageInstallationLevelAuditAllUsers")) == 0);
	CTypedPtrList<CObList, CuNameOnly*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuNameOnly*>*)lParam;
	ResetDisplay();

	CuNameOnly* pObj = NULL;
	try
	{
		while (!pListTuple->IsEmpty())
		{
			pObj = (CuNameOnly*)pListTuple->RemoveHead();
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

LONG CuDlgDomPropInstallationLevelAuditAllUsers::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuNameOnly* p = NULL;
	CuDomPropDataPageInstallationLevelAuditAllUsers* pData = NULL;
	try
	{
		pData = new CuDomPropDataPageInstallationLevelAuditAllUsers ();
		CTypedPtrList<CObList, CuNameOnly*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			CuNameOnly* pItem = (CuNameOnly*)m_cListCtrl.GetItemData(i);
			if (!pItem)
				continue;
			p = new CuNameOnly (pItem->GetStrName(), FALSE);
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

void CuDlgDomPropInstallationLevelAuditAllUsers::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}

void CuDlgDomPropInstallationLevelAuditAllUsers::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CuNameOnly* p = (CuNameOnly*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (p != NULL)
		delete p;
	*pResult = 0;
}

void CuDlgDomPropInstallationLevelAuditAllUsers::AddItem (LPCTSTR lpszItem, CuNameOnly* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CuDlgDomPropInstallationLevelAuditAllUsers::DisplayItems()
{
	CuNameOnly* p = NULL;
	int i, nCount = m_cListCtrl.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		p = (CuNameOnly*)m_cListCtrl.GetItemData (i);
		if (!p)
			continue;
		m_cListCtrl.SetItemText (i, 0, p->GetStrName());
	}
}

void CuDlgDomPropInstallationLevelAuditAllUsers::ResetDisplay()
{
	m_cListCtrl.DeleteAllItems();
	VDBA_OnGeneralSettingChange(&m_cListCtrl);
	// Force immediate update of all children controls
	RedrawAllChildWindows(m_hWnd);
}