/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : psheet.cpp : implementation file
**  Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : CuPropertySheet is a modeless property sheet that is 
**             created once and not destroyed until the owner is being destroyed.
**             It is initialized and controlled from CfMiniFrame.
**
** History :
**
** 06-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    created
** 07-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#include "stdafx.h"
#include "psheet.h"
#include "fminifrm.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CuPropertySheet, CPropertySheet)

CuPropertySheet::CuPropertySheet(CWnd* pWndParent)
	 : CPropertySheet(_T("You must set the title"), pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the 
	// active one is to call SetActivePage().

	AddPage(&m_Page1);
}

CuPropertySheet::CuPropertySheet(CWnd* pWndParent, CTypedPtrList < CObList, CPropertyPage* >& listPages)
	: CPropertySheet(_T("You must set the title"), pWndParent)
{
	POSITION pos = listPages.GetHeadPosition();
	while (pos != NULL)
	{
		CPropertyPage* pPage = listPages.GetNext(pos);
		AddPage(pPage);
	}
}


CuPropertySheet::~CuPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CuPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CuPropertySheet)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_UPDATEDATA, OnUpdateData)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuPropertySheet message handlers

void CuPropertySheet::PostNcDestroy()
{
	CPropertySheet::PostNcDestroy();
	delete this;
}



void CuPropertySheet::OnSize(UINT nType, int cx, int cy) 
{
	CPropertySheet::OnSize(nType, cx, cy);
	//
	// Resize the CTabCtrl:
	CRect rectWindow;
	CTabCtrl* pTab = GetTabControl();
	if (!pTab)
		return;
	CRect r;
	GetClientRect (r);
	pTab->MoveWindow(r);
	//
	// Resize the active page
	CPropertyPage* pPage = GetActivePage();
	if (!pPage)
		return;
	if (!IsWindow (pPage->m_hWnd))
		return;

	pTab->GetClientRect (r);
	pTab->AdjustRect (FALSE, r);
	pPage->MoveWindow(r);
}

int CuPropertySheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}


long CuPropertySheet::OnUpdateData(WPARAM w, LPARAM l)
{
	CPropertyPage* pPage = GetActivePage();
	if (!pPage)
		return 0;
	pPage->SendMessage (WMUSRMSG_UPDATEDATA, w, l);
	return 0;
}
