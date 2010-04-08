/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : xdlgpref.cpp , Implementation File
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The Preferences setting of Visual Manager
**
**
** History:
**
** 05-Feb-1999 (uk$so01)
**    Created
** 27-Jan-2000 (uk$so01)
**    (Bug #100157): Activate Help Topic
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**
**/

#include "stdafx.h"
#include "ivm.h"
#include "xdlgpref.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CxDlgPropertySheetPreference, CPropertySheet)

CxDlgPropertySheetPreference::CxDlgPropertySheetPreference(CWnd* pWndParent)
	 : CPropertySheet(IDS_PROPSHT_PREFERENCE_CAPTION, pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the 
	// active one is to call SetActivePage().

//	AddPage(&m_Page1);
	AddPage(&m_Page2);
}

CxDlgPropertySheetPreference::~CxDlgPropertySheetPreference()
{
}


BEGIN_MESSAGE_MAP(CxDlgPropertySheetPreference, CPropertySheet)
	//{{AFX_MSG_MAP(CxDlgPropertySheetPreference)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CxDlgPropertySheetPreference::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nHelpID = 0;
	CPropertyPage*  pCurrentPage = (CPropertyPage* )GetActivePage();
	if (pCurrentPage == &m_Page2)
		nHelpID = IDD_PROPPAGE_EVENT_NOTIFY;
	return APP_HelpInfo(nHelpID);
}
