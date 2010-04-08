/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iapsheet.cpp : implementation file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : This class defines custom modal property sheet 
**                CuIEAPropertySheet.
**
** History:
**
** 15-Oct-2001 (uk$so01)
**    Created
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 08-May-2008 (drivi01)
**    Apply wizard97 style to this wizard.
**/

#include "stdafx.h"
#include "resource.h"
#include "eapsheet.h"
#include "afxpriv.h"
#include "rctools.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNAMIC(CuIEAPropertySheet, CPropertySheet)
CuIEAPropertySheet::CuIEAPropertySheet(CWnd* pWndParent)
	 : CPropertySheet(AFX_IDS_APP_TITLE, pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the 
	// active one is to call SetActivePage().
	m_psh.dwFlags |= PSH_HASHELP|PSH_PROPSHEETPAGE;
	m_psh.nPages = 2;

	AddPage(&m_Page1);
	AddPage(&m_Page2Csv);

	SetPreviousPage (0);
	SetWizardMode();

	m_pStruct = NULL;

	m_psh.dwFlags |= PSH_WIZARD97|PSH_USEHBMHEADER|PSH_WATERMARK ;

	m_psh.pszbmWatermark = MAKEINTRESOURCE(IDB_IMPORTAS_2);
	m_psh.hbmHeader = NULL;

	m_psh.hInstance = AfxGetInstanceHandle(); 
}

CuIEAPropertySheet::~CuIEAPropertySheet()
{
}

BOOL CuIEAPropertySheet::OnInitDialog() 
{
	BOOL bOk = CPropertySheet::OnInitDialog();

	CWnd* pHelpButton = GetDlgItem (ID_W_HELP);
	ASSERT (pHelpButton);
	if (pHelpButton)
		pHelpButton->EnableWindow (TRUE);

	return bOk;
}

void CuIEAPropertySheet::OnHelp()
{
	CaIEAInfo& data = GetData();
	CaIeaDataPage1& dataPage1 = data.m_dataPage1;
	CTabCtrl* pTab = GetTabControl();
	ASSERT(pTab);
	if (pTab)
	{
		UINT nHelpID = 0;
		int nActivePage = pTab->GetCurSel();
		switch (nActivePage)
		{
		case 0:
			nHelpID = 105;
			break;
		case 1:
			switch (dataPage1.GetExportedFileType())
			{
			case EXPORT_CSV:
				nHelpID = 106;
				break;
			case EXPORT_FIXED_WIDTH:
				nHelpID = 107;
				break;
			case EXPORT_DBF:
				nHelpID = 108;
				break;
			case EXPORT_XML:
				nHelpID = 109;
				break;
			case EXPORT_COPY_STATEMENT:
			default:
				break;
			}
			break;
		default:
			nHelpID = 105;
			break;
		}
		APP_HelpInfo(m_hWnd, nHelpID);
	}
}

int CuIEAPropertySheet::DoModal()
{
	try
	{
		int nRes = CPropertySheet::DoModal();
		return nRes;
	}
	catch(...)
	{
		return IDCANCEL;
	}
}



BEGIN_MESSAGE_MAP(CuIEAPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CuIEAPropertySheet)
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuIEAPropertySheet message handlers


void CuIEAPropertySheet::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertySheet::OnShowWindow(bShow, nStatus);
	CRect r;
	int nWidth  = GetSystemMetrics(SM_CXSCREEN);
	int nHeigth = GetSystemMetrics(SM_CYSCREEN);
	nWidth = nWidth/2;
	nHeigth= nHeigth/2;
	GetWindowRect (r);
	nWidth = nWidth - r.Width()/2;
	nHeigth= nHeigth -r.Height()/2;
	if (nWidth < 0)
		nWidth = 0;
	if (nHeigth < 0)
		nHeigth = 0;

	SetWindowPos (&wndTop, nWidth, nHeigth, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
}



LRESULT CuIEAPropertySheet::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	//
	// The HELP Button is always disable. This is the reason why I overwrite this
	// function.
	// Due to the documentation, if PSH_HASHELP is set for the page (in property page,
	// the member m_psp.dwFlags |= PSH_HASHELP) then the help button is enable. But it
	// does not work.

	CWnd* pHelpButton = GetDlgItem (ID_W_HELP);
	if (pHelpButton)
		pHelpButton->EnableWindow (TRUE);
	
	return CPropertySheet::WindowProc(message, wParam, lParam);
}

void CuIEAPropertySheet::OnDestroy() 
{
	theApp.m_sessionManager.Cleanup();
	CPropertySheet::OnDestroy();
}
