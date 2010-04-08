/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : parinsta.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parameter Page for Installation branch
**
** History:
**
** 16-Dec-1998 (uk$so01)
**    created
** 28-Feb-2000 (uk$so01)
**    Created. (SIR # 100634)
**    The "Add" button of Parameter Tab can be used to set 
**    Parameters for System and User Tab. 
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 06-Mar-2000 (uk$so01)
**    SIR #100751, Remove the button "Restore" of Parameter Tab.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
**
**/

#include "stdafx.h"
#include "ivm.h"
#include "parinsta.h"
#include "mgrfrm.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgParameterInstallation dialog


CuDlgParameterInstallation::CuDlgParameterInstallation(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgParameterInstallation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgParameterInstallation)
	m_bCheckShowUnsetParameter = FALSE;
	//}}AFX_DATA_INIT
	m_pDlgParameterSystem = NULL;
	m_pDlgParameterUser = NULL;
	m_pDlgParameterExtra = NULL;
	m_pCurrentPage = NULL;
}


void CuDlgParameterInstallation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgParameterInstallation)
	DDX_Control(pDX, IDC_CHECK1, m_cCheckShowUnsetParameter);
	DDX_Control(pDX, IDC_BUTTON4, m_cButtonAdd);
	DDX_Control(pDX, IDC_BUTTON3, m_cButtonUnset);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonEditValue);
	DDX_Control(pDX, IDC_TAB1, m_cTab1);
	DDX_Check(pDX, IDC_CHECK1, m_bCheckShowUnsetParameter);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgParameterInstallation, CDialog)
	//{{AFX_MSG_MAP(CuDlgParameterInstallation)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnSelchangeTab1)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonEditValue)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonRestore)
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonUnset)
	ON_BN_CLICKED(IDC_BUTTON4, OnButtonAdd)
	ON_BN_CLICKED(IDC_CHECK1, OnCheckShowUnsetParameter)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_ENABLE_CONTROL, OnEnableControl)
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING,   OnUpdateData)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgParameterInstallation message handlers

void CuDlgParameterInstallation::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgParameterInstallation::OnInitDialog() 
{
	CDialog::OnInitDialog();
	//
	// Construct the Tab Control:
	const int NTAB = 3;
	UINT nTab [NTAB] = {IDS_HEADER_SYSTEM, IDS_HEADER_USER, IDS_HEADER_EXTRA};

	CRect   r;
	TC_ITEM item;
	memset (&item, 0, sizeof (item));
	item.mask       = TCIF_TEXT;
	item.cchTextMax = 32;
	CString strHeader;
	for (int i=0; i<NTAB; i++)
	{
		strHeader.LoadString (nTab[i]);
		item.pszText = (LPTSTR)(LPCTSTR)strHeader;
		m_cTab1.InsertItem (i, &item);
	}


	m_pDlgParameterSystem = new CuDlgParameterInstallationSystem (&m_cTab1);
	m_pDlgParameterUser = new CuDlgParameterInstallationUser (&m_cTab1);
	m_pDlgParameterExtra = new CuDlgParameterInstallationExtra (&m_cTab1);
	m_pDlgParameterSystem->Create (IDD_PARAMETER_INSTALLATION_SYSTEM, &m_cTab1);
	m_pDlgParameterUser->Create (IDD_PARAMETER_INSTALLATION_USER, &m_cTab1);
	m_pDlgParameterExtra->Create (IDD_PARAMETER_INSTALLATION_EXTRA, &m_cTab1);

	m_cTab1.SetCurSel (0);
	SelchangeTab1();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgParameterInstallation::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CRect r, r2, rDlg;
	GetClientRect (rDlg);
	if (rDlg.Height() < 100)
		return;

	if (IsWindow (m_cTab1))
	{
		//
		// Resize the Buttons (Edit Value + Restore + Unset):
		int H;
		m_cButtonEditValue.GetWindowRect (r);
		ScreenToClient (r);
		H = r.Height();
		r.bottom = rDlg.bottom - 4;
		r.top    = r.bottom - H;
		m_cButtonEditValue.MoveWindow (r);

		m_cButtonUnset.GetWindowRect (r);
		ScreenToClient (r);
		H = r.Height();
		r.bottom = rDlg.bottom - 4;
		r.top    = r.bottom - H;
		m_cButtonUnset.MoveWindow (r);

		m_cButtonAdd.GetWindowRect (r);
		ScreenToClient (r);
		H = r.Height();
		r.bottom = rDlg.bottom - 4;
		r.top    = r.bottom - H;
		m_cButtonAdd.MoveWindow (r);

		m_cCheckShowUnsetParameter.GetWindowRect (r);
		ScreenToClient (r);
		H = r.Height();
		r.bottom = rDlg.bottom - 4;
		r.top    = r.bottom - H;
		m_cCheckShowUnsetParameter.MoveWindow (r);

		//
		// Resize the Tab Control:
		m_cButtonEditValue.GetWindowRect (r2);
		ScreenToClient (r2);

		m_cTab1.GetWindowRect (r);
		ScreenToClient (r);
		r.left   = rDlg.left  + 2;
		r.right  = rDlg.right - 2;
		r.bottom = r2.top - 4;
		m_cTab1.MoveWindow (r);

		SelchangeTab1(FALSE);
	}
}

void CuDlgParameterInstallation::SelchangeTab1(BOOL bUpdateData)
{
	CRect r;
	int nSel = m_cTab1.GetCurSel();

	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
		m_pCurrentPage->ShowWindow (SW_HIDE);
	switch (nSel)
	{
	case 0:
		m_pCurrentPage = m_pDlgParameterSystem;
		break;
	case 1:
		m_pCurrentPage = m_pDlgParameterUser;
		break;
	case 2:
		m_pCurrentPage = m_pDlgParameterExtra;
		break;
	default:
		m_pCurrentPage = m_pDlgParameterSystem;
		break;
	}
	m_cTab1.GetClientRect (r);
	m_cTab1.AdjustRect (FALSE, r);
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->MoveWindow (r);
		m_pCurrentPage->ShowWindow(SW_SHOW);
		if (bUpdateData)
			OnUpdateData(0, 0);
	}
	//
	// Enable / Disable buttons:
	// (Request the page to send the message back):
	m_pCurrentPage->PostMessage (WMUSRMSG_ENABLE_CONTROL, 0, 0);
}


void CuDlgParameterInstallation::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SelchangeTab1();
	*pResult = 0;
}

void CuDlgParameterInstallation::OnButtonEditValue() 
{
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
		m_pCurrentPage->SendMessage (WMUSRMSG_LISTCTRL_EDITING, 0, 0);
}

void CuDlgParameterInstallation::OnButtonRestore() 
{
	// TODO: Add your control notification handler code here
	
}

void CuDlgParameterInstallation::OnButtonUnset() 
{
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
		m_pCurrentPage->SendMessage (WMUSRMSG_VARIABLE_UNSET, 0, 0);
}


LONG CuDlgParameterInstallation::OnEnableControl(WPARAM w, LPARAM l)
{
	UINT nState = (UINT)w;
	m_cButtonEditValue.EnableWindow ((BOOL)(nState & ST_MODIFY));
	m_cButtonUnset.EnableWindow ((BOOL)(nState & ST_UNSET));
	m_cButtonAdd.EnableWindow ((BOOL)(nState & ST_ADD));

	return 0;
}

void CuDlgParameterInstallation::OnButtonAdd() 
{
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		int nCheck = m_cCheckShowUnsetParameter.GetCheck();
		m_pCurrentPage->SendMessage (WMUSRMSG_ADD_OBJECT, (WPARAM)nCheck);
	}
}



void CuDlgParameterInstallation::OnCheckShowUnsetParameter() 
{
	int nCheck = m_cCheckShowUnsetParameter.GetCheck();
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)nCheck);
	}

}

LONG CuDlgParameterInstallation::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int nCheck = m_cCheckShowUnsetParameter.GetCheck();
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		m_pCurrentPage->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)nCheck);
	}

	return 0;
}

LONG CuDlgParameterInstallation::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}


LONG CuDlgParameterInstallation::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgParameterInstallation::OnFind \n");
	if (m_pCurrentPage && IsWindow (m_pCurrentPage->m_hWnd))
	{
		return (long)m_pCurrentPage->SendMessage (WMUSRMSG_FIND, wParam, lParam);
	}

	return 0L;
}
