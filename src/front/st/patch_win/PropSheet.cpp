/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: PropSheet.cpp : implementation file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	13-jun-2001 (somsa01)
**	    Modified the removing of the HELP button.
**	01-Mar-2006 (drivi01)
**	    Reused for MSI Patch Installer on Windows.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "PropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropSheet

IMPLEMENT_DYNAMIC(CPropSheet, CPropertySheet)

CPropSheet::CPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	AddPage(new CWelcome());
	AddPage(new CInstallCode());
	SetWizardMode();
	m_psh.dwFlags &= ~(PSH_HASHELP);
}


CPropSheet::~CPropSheet()
{
	for (int i=0; i<GetPageCount(); i++)
		delete GetPage(i);
}


BEGIN_MESSAGE_MAP(CPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPropSheet)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropSheet message handlers

BOOL CPropSheet::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch(wParam)
	{
	case IDCANCEL:
		if (!AskUserYN(IDS_AREYOUSURE))
			return TRUE;
	}

	return CPropertySheet::OnCommand(wParam, lParam);
}

BOOL CPropSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	SetIcon(theIcon, TRUE);
	SetIcon(theIcon, FALSE);
	return bResult;
}

void CPropSheet::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CPropertySheet::OnActivate(nState, pWndOther, bMinimized);

	CString s;
	s.LoadString(IDS_TITLE);
	SetWindowText(s);	
}

void CPropSheet::OnClose() 
{
	if(AskUserYN(IDS_AREYOUSURE))
		CPropertySheet::OnClose();
}

void CPropSheet::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); /* device context for painting */
		
		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
		
		/* Center icon in client rectangle */
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		
		/* Draw the icon */
		dc.DrawIcon(x, y, theIcon);
	}
	else
	{
		CPropertySheet::OnPaint();
	}
}

HCURSOR CPropSheet::OnQueryDragIcon() 
{
	return (HCURSOR) theIcon;
}
