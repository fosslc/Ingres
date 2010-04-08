/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xmaphost.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Popup dialog for mapping host functionality
**
** History:
**
** 11-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/


#include "stdafx.h"
#include "vcda.h"
#include "xmaphost.h"
#include "vcdadml.h"
#include "udlgmain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgHostMapping dialog
static void FillHosts(CListBox* pLb, CStringList* listItem)
{
	if (!listItem)
		return;
	if (!pLb)
		return;
	if (listItem->IsEmpty())
		return;
	POSITION pos = listItem->GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = listItem->GetNext(pos);
		strItem.MakeLower();
		pLb->AddString(strItem);
	}
}

static void FillMappedHosts(CListCtrl* pLb, CTypedPtrList< CObList, CaMappedHost* >* pListItem)
{
	if (!pListItem)
		return;
	if (!pLb)
		return;
	if (pListItem->IsEmpty())
		return;
	POSITION pos = pListItem->GetHeadPosition();
	while (pos != NULL)
	{
		CaMappedHost* pItem = pListItem->GetNext(pos);
		int n = pLb->InsertItem(pLb->GetItemCount(), pItem->GetHost1());
		if (n != -1)
			pLb->SetItemText(n, 1, pItem->GetHost2());
	}
}

CxDlgHostMapping::CxDlgHostMapping(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgHostMapping::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgHostMapping)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pstrList1Host = NULL;
	m_pstrList2Host = NULL;
	m_plistMappedHost = NULL;
	m_strSnapshot1 = _T("???");
	m_strSnapshot2 = _T("???");
}


void CxDlgHostMapping::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgHostMapping)
	DDX_Control(pDX, IDC_STATIC_H2, m_cStaticH2);
	DDX_Control(pDX, IDC_STATIC_H1, m_cStaticH1);
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonRemove);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonMap);
	DDX_Control(pDX, IDC_LIST2, m_cList2Host);
	DDX_Control(pDX, IDC_LIST1, m_cList1Host);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgHostMapping, CDialog)
	//{{AFX_MSG_MAP(CxDlgHostMapping)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_LBN_SELCHANGE(IDC_LIST2, OnSelchangeList2)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST3, OnItemchangedList3)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonMap)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonRemove)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgHostMapping message handlers

void CxDlgHostMapping::OnSelchangeList1() 
{
	// TODO: Add your control notification handler code here
	
}

void CxDlgHostMapping::OnSelchangeList2() 
{
	// TODO: Add your control notification handler code here
	
}

void CxDlgHostMapping::OnItemchangedList3(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

BOOL CxDlgHostMapping::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST3, this));
	
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[2] =
	{
		{IDS_TAB_SNAPSHOT1, 200,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_SNAPSHOT2, 200,  LVCFMT_LEFT,  FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, 2);
	FillHosts(&m_cList1Host, m_pstrList1Host);
	FillHosts(&m_cList2Host, m_pstrList2Host);
	FillMappedHosts(&m_cListCtrl, m_plistMappedHost);

	LVCOLUMN vc;
	memset (&vc, 0, sizeof (vc));
	vc.mask = LVCF_TEXT;
	vc.pszText = (LPTSTR)(LPCTSTR)m_strSnapshot1;
	m_cListCtrl.SetColumn(0, &vc);
	vc.pszText = (LPTSTR)(LPCTSTR)m_strSnapshot2;
	m_cListCtrl.SetColumn(1, &vc);

	m_cStaticH1.SetWindowText(m_strSnapshot1);
	m_cStaticH2.SetWindowText(m_strSnapshot2);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgHostMapping::OnButtonMap() 
{
	CString strHost1;
	CString strHost2;
	int nSel1 = m_cList1Host.GetCurSel();
	int nSel2 = m_cList2Host.GetCurSel();
	if (nSel1 != LB_ERR)
		m_cList1Host.GetText(nSel1, strHost1);
	if (nSel2 != LB_ERR)
		m_cList2Host.GetText(nSel2, strHost2);

	if (nSel1 != LB_ERR && nSel2 != LB_ERR)
	{
		if (!strHost1.IsEmpty() && !strHost2.IsEmpty())
		{
			int n = m_cListCtrl.InsertItem(m_cListCtrl.GetItemCount(), strHost1);
			if (n != -1)
			{
				m_cListCtrl.SetItemText(n, 1, strHost2);
				m_cList1Host.DeleteString(nSel1);
				m_cList2Host.DeleteString(nSel2);
			}
		}
	}
}

void CxDlgHostMapping::OnButtonRemove() 
{
	int nSelected = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
	if (nSelected != -1)
	{
		CString strHost1 = m_cListCtrl.GetItemText (nSelected, 0);
		CString strHost2 = m_cListCtrl.GetItemText (nSelected, 1);

		m_cList1Host.AddString(strHost1);
		m_cList2Host.AddString(strHost2);
		m_cListCtrl.DeleteItem(nSelected);
	}
}

void CxDlgHostMapping::OnOK() 
{
	CDialog::OnOK();

	CString strItem;
	m_pstrList1Host->RemoveAll();
	m_pstrList2Host->RemoveAll();
	while (!m_plistMappedHost->IsEmpty())
		delete m_plistMappedHost->RemoveHead();

	int i, n = m_cListCtrl.GetItemCount();
	int n1 = m_cList1Host.GetCount();
	int n2 = m_cList2Host.GetCount();
	for (i=0; i< n1; i++)
	{
		m_cList1Host.GetText (i, strItem);
		m_pstrList1Host->AddTail(strItem);
	}
	for (i=0; i< n2; i++)
	{
		m_cList2Host.GetText (i, strItem);
		m_pstrList2Host->AddTail(strItem);
	}
	for (i=0; i<n; i++)
	{
		CString strHost1 = m_cListCtrl.GetItemText (i, 0);
		CString strHost2 = m_cListCtrl.GetItemText (i, 1);
		m_plistMappedHost->AddTail (new CaMappedHost(strHost1, strHost2));
	}
}

BOOL CxDlgHostMapping::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	APP_HelpInfo(m_hWnd, 40020);
	return CDialog::OnHelpInfo(pHelpInfo);
}

