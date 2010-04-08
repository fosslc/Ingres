/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : mtabdlg.cpp, Implementation file 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut, SCHALK P
**    Purpose  : Modeless Dialog holds 3 Tabs: 
**               (History of Changes, Configure, Preferences) 
**               See the CONCEPT.DOC for more detail
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    created
** 30-mar-1999 (cucjo01)
**    Remove the preferences tab.
** 10-jun-1999 (mcgem01)
      EDBC has its own independent help file.
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 06-Nov-2001 (loera01) Bug 106295
**    Path references to help files should be in lower case for Unix.
** 09-Feb-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**/

#include "stdafx.h"
#include "vcbf.h"
#include "mtabdlg.h"
#include "histdlg.h"
#include "prefdlg.h"
#include "cconfdlg.h"
#include "wmusrmsg.h"
#include "mainfrm.h"
#include "ll.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined (OIDSK)
#define VCBF_HELP_ 10500 // OIDT  Help Contexts IDs start from this value.
#else
#define VCBF_HELP_ 10000 // OPING Help Contexts IDs start from this value.
#endif
/////////////////////////////////////////////////////////////////////////////
// CMainTabDlg dialog

void CMainTabDlg::SetHeader (LPCTSTR lpszInstall, LPCTSTR lpszHost, LPCTSTR lpszIISyst)
{
	m_strHeader.Format (_T("Installation: %s	Host: %s	IIsystem: %s"), lpszInstall, lpszHost, lpszIISyst);
	UpdateData (FALSE);
}

CMainTabDlg::CMainTabDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMainTabDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMainTabDlg)
	m_strHeader = _T("Installation: II	Host: FRNAGP10	IIsystem: D:\\OPING20");
	//}}AFX_DATA_INIT
	m_pCurDlg= (CDialog *)0;

	m_pConfigDlg = NULL;
	m_pHistDlg = NULL;
	m_pPrefDlg = NULL;
}


void CMainTabDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainTabDlg)
	DDX_Text(pDX, IDC_STATIC1, m_strHeader);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMainTabDlg, CDialog)
	//{{AFX_MSG_MAP(CMainTabDlg)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonOK)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonCancel)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainTabDlg message handlers

BOOL CMainTabDlg::OnInitDialog() 
{
	CRect   r;
	TC_ITEM item;
	CString strTab;
	UINT nTabID[3]= {IDS_TAB_MAIN_CONFIGURE,IDS_TAB_MAIN_HISTORY,IDS_TAB_MAIN_PREFERENCES};
	CDialog::OnInitDialog();
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT|TCIF_IMAGE;
	item.cchTextMax = 32;
	item.iImage     = -1;
	CTabCtrl * ptab = (CTabCtrl * )GetDlgItem(IDC_TAB1);
	ASSERT(ptab);

	ptab->GetWindowRect (r);
	ScreenToClient (r);
	m_dlgPadding.cx = r.left;
	m_dlgPadding.cy = r.top;

	CButton* pButton1= (CButton*)GetDlgItem(IDC_BUTTON1);
	ASSERT (pButton1);
	pButton1->GetWindowRect (r);
	ScreenToClient (r);
	m_buttonPadding.cx = r.right;
	m_buttonPadding.cy = r.bottom;
	int nTab = 2;
#if defined (OIDSK)
	nTab = 2;
#endif
	for (int i=0; i<nTab; i++)  {
		strTab.LoadString (nTabID [i]);
		item.pszText = (LPTSTR)(LPCTSTR)strTab;
		ptab->InsertItem (i, &item);
	}
	if (m_pCurDlg)
		m_pCurDlg->ShowWindow (SW_HIDE);
	CTabCtrl* pTab = (CTabCtrl*) GetDlgItem(IDC_TAB1);
	int nsel = pTab->GetCurSel();
	switch (nsel) 
	{
	case PANE_CONFIG:
		if (!m_pConfigDlg)
		{
			m_pConfigDlg = new CConfigDlg(pTab);
			m_pConfigDlg->Create(IDD_CONFIG,pTab);
		}
		m_pCurDlg= m_pConfigDlg;
		break;
	case PANE_HISTORY:
		if (!m_pHistDlg)
		{
			m_pHistDlg = new CHistDlg(pTab);
			m_pHistDlg->Create(IDD_HISTORY,pTab);
		}
		m_pCurDlg= m_pHistDlg;
		break;
	case PANE_PREFERENCES:
		if (!m_pPrefDlg)
		{
			m_pPrefDlg = new CPrefDlg(pTab);
			m_pPrefDlg->Create(IDD_PREFERENCES,pTab);
		}
		m_pCurDlg= m_pPrefDlg;
		break;
	} 

	if (m_pCurDlg) 
	{
		CRect r;
		pTab->GetClientRect (r);
		pTab->AdjustRect (FALSE, r);
		m_pCurDlg->MoveWindow(r);
		m_pCurDlg->ShowWindow(SW_SHOW);
	}
	SHOSTINFO CurInfo;
	
	VCBFGetHostInfos(&CurInfo);
	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_STATIC_NAME_INSTALL);
	pStatic->SetWindowText((LPCTSTR)CurInfo.ii_installation_name);
	
	pStatic = (CStatic*)GetDlgItem(IDC_STATIC_INSTALL);
	pStatic->SetWindowText((LPCTSTR)CurInfo.ii_installation);
	
	
	pStatic = (CStatic*)GetDlgItem(IDC_STATIC_IISYSTEM_NAME);
	pStatic->SetWindowText((LPCTSTR)CurInfo.ii_system_name);
	
	pStatic = (CStatic*)GetDlgItem(IDC_STATIC_II_SYSTEM);
	pStatic->SetWindowText((LPCTSTR)CurInfo.ii_system);
	
	pStatic = (CStatic*)GetDlgItem(IDC_STATIC_HOST);
	pStatic->SetWindowText((LPCTSTR)CurInfo.host);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMainTabDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CTabCtrl * pTab = (CTabCtrl *) GetDlgItem(IDC_TAB1);
	CRect rDlg;

	GetClientRect (rDlg);
	if (pTab && IsWindow(pTab->m_hWnd)) 
	{
		//
		// Move the buttons to the right place.
		CButton* pB1 = (CButton*) GetDlgItem(IDC_BUTTON2); // Close;
		CButton* pB2 = (CButton*) GetDlgItem(IDC_BUTTON1); // Help
		CButton* pB3 = (CButton*) GetDlgItem(IDC_BUTTON3);
		
		int   ibY = 0;
		CRect r, newRect;
		pB1->GetClientRect (r);
		newRect.top    = rDlg.bottom - 10 - r.Height();
		newRect.right  = rDlg.right  - m_dlgPadding.cx;
		newRect.left   = newRect.right - r.Width();
		newRect.bottom = newRect.top + r.Height();
		pB1->MoveWindow (newRect);

		newRect.right  = newRect.left - 10;
		newRect.left   = newRect.right - r.Width();
		pB2->MoveWindow (newRect);

		newRect.right  = newRect.left - 10;
		newRect.left   = newRect.right - r.Width();
		pB3->MoveWindow (newRect);
		ibY = newRect.top;

		//
		// Move the static Text Header II_SYSTEM 
		// to the right place.
		CStatic* pStatic = (CStatic*)GetDlgItem(IDC_STATIC_II_SYSTEM);
		pStatic->GetWindowRect (r);
		ScreenToClient (r);
		r.right   = rDlg.right  - m_dlgPadding.cx;
		pStatic->MoveWindow (r);


		//
		// Move the Tab Control to the right place.
		pStatic->GetWindowRect (r);
		ScreenToClient (r);

		r.top     = r.bottom + 10;
		r.bottom  = ibY - 10;
		r.left    = m_dlgPadding.cx;
		r.right   = rDlg.right -  m_dlgPadding.cx;
		pTab->MoveWindow(r);
		if (m_pCurDlg) 
		{
			CRect r;
			pTab->GetClientRect (r);
			pTab->AdjustRect (FALSE, r);
			m_pCurDlg->MoveWindow(r);
		}
	}
}

void CMainTabDlg::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CMainTabDlg::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if (m_pCurDlg) 
	{
		//
		// Try to save (and validate) the data that has been changed
		// in the old page before displaying a new page.
		m_pCurDlg->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
		m_pCurDlg->ShowWindow(SW_HIDE);
		m_pCurDlg->SendMessage (WM_COMPONENT_EXIT, 0, 0);
		m_pCurDlg = NULL;
	}
	
	CTabCtrl* pTab = (CTabCtrl*) GetDlgItem(IDC_TAB1);
	int nsel = pTab->GetCurSel();
	
	switch (nsel) 
	{
	case  PANE_CONFIG:
		if (!m_pConfigDlg)
		{
			m_pConfigDlg = new CConfigDlg(pTab);
			m_pConfigDlg->Create(IDD_CONFIG,pTab);
		}
		m_pCurDlg= m_pConfigDlg;
		break;
	case PANE_HISTORY:
		if (!m_pHistDlg)
		{
			m_pHistDlg = new CHistDlg(pTab);
			m_pHistDlg->Create(IDD_HISTORY,pTab);
		}
		m_pCurDlg= m_pHistDlg;
		break;
	case PANE_PREFERENCES:
		if (!m_pPrefDlg)
		{
			m_pPrefDlg = new CPrefDlg(pTab);
			m_pPrefDlg->Create(IDD_PREFERENCES,pTab);
		}
		m_pCurDlg= m_pPrefDlg;
		break;
	} 
	
	if (m_pCurDlg) 
	{
		CRect r;
		pTab->GetClientRect (r);
		pTab->AdjustRect (FALSE, r);
		m_pCurDlg->MoveWindow(r);
		m_pCurDlg->ShowWindow(SW_SHOW);
		m_pCurDlg->SendMessage(WMUSRMSG_CBF_PAGE_UPDATING, 0, 0);
		m_pCurDlg->SendMessage(WM_COMPONENT_ENTER, 0, 0);
	}
}

void CMainTabDlg::OnDestroy() 
{
	if (m_pConfigDlg)
		m_pConfigDlg->DestroyWindow();
	if (m_pHistDlg)
		m_pHistDlg->DestroyWindow();
	if (m_pPrefDlg)
		m_pPrefDlg->DestroyWindow();
}

void CMainTabDlg::OnButtonOK() 
{
	TRACE0 ("CMainTabDlg::OnButtonOK()...\n");	
}

void CMainTabDlg::OnButtonCancel() 
{
	CWnd* pParent1 = GetParent ();             // View (CVcbfView)
	ASSERT_VALID (pParent1);
	CWnd* pParent2 = pParent1->GetParent ();   // Frame(CMainFrame)
	ASSERT_VALID (pParent2);
	if (m_pCurDlg) 
	{
		//
		// Try to save (and validate) the data that has been changed
		// in the old page before displaying a new page.
		m_pCurDlg->SendMessage(WMUSRMSG_CBF_PAGE_VALIDATE, (WPARAM)0, (LPARAM)0);
	}
	/*
	CString strMsg = "You will close the application.\nDo you wish to save changes ?";
	int nID = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNOCANCEL);
	if (nID == IDCANCEL)
		return;
	BOOL bSave = (nID == IDYES)? TRUE: FALSE;
	*/
	((CMainFrame*)pParent2)->CloseApplication (TRUE);   // False reserved for exceptions urgent termination
	TRACE0 ("CMainTabDlg::OnButtonCancel()...\n");	
}

void CMainTabDlg::OnButtonHelp() 
{
	UINT nHelpID = GetHelpID();
	TRACE1 ("CMainTabDlg::OnButtonHelp(), Help Context ID = %d\n", nHelpID);
	APP_HelpInfo(m_hWnd, nHelpID);
}


UINT CMainTabDlg::GetHelpID()
{
	CTabCtrl* pTab = (CTabCtrl*) GetDlgItem(IDC_TAB1);
	int nsel = pTab->GetCurSel();
	UINT nHelpID = 0;
	
	switch (nsel) 
	{
	case  PANE_CONFIG:
		ASSERT (m_pConfigDlg != NULL);
		nHelpID = m_pConfigDlg? m_pConfigDlg->GetHelpID(): 0;
		return (nHelpID == 0)? (IDD_CONFIG_LEFT+VCBF_HELP_): (nHelpID+VCBF_HELP_);
	case PANE_HISTORY:
		ASSERT (m_pHistDlg != NULL);
		return (IDD_HISTORY + VCBF_HELP_);
	case PANE_PREFERENCES:
		ASSERT (m_pPrefDlg != NULL);
		return (IDD_PREFERENCES + VCBF_HELP_);
	default:
		return nHelpID;
	} 
}
