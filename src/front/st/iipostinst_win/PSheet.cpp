/*
** Copyright (c) 2001 Ingres Corporation
*/

/*
**
** Name: PropSheet.cpp
**
** Description:
**      This file contains the routines for implememting the application's
**      Property sheet.
**
** History:
**      10-apr-2001 (somsa01)
**          Created.
**	17-may-2001 (somsa01)
**	    Instead of centering the main window before it's time, just hide
**	    it and show it when it is time.
**	13-jun-2001 (somsa01)
**	    Removed CANCEL feature from post installer, as well as
**	    OnCommand() and OnClose().
**	24-oct-2001 (penga03)
**	    If there exists system menu, disable and gray it out.
**	06-mar-2002 (penga03)
**	    Set the proper title for evaluation releases.
**  15-Nov-2006 (drivi01)
**      SIR 116599
**      Enhanced post-installer in effort to improve installer usability.
**		Added a summary page to the property sheet, updated wizard with
**		WIZARD97 template.
**  04-Apr-2008 (drivi01)
**		Adding functionality for DBA Tools installer to the Ingres code.
**		DBA Tools installer is a simplified package of Ingres install
**		containing only NET and Visual tools component.  
**		Adjust title of the post installer for DBA Tools installation.
*/

#include "stdafx.h"
#include "iipostinst.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CPropSheet
*/

IMPLEMENT_DYNAMIC(CPropSheet, CPropertySheet)

CPropSheet::CPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    AddPage(new CPostInst());
	AddPage(new Summary());
	SetWizardMode();
    m_psh.dwFlags=(m_psh.dwFlags&(~PSH_HASHELP));
	m_psh.dwFlags |= PSH_WIZARD97|PSH_HEADER|PSH_USEHBMHEADER|PSH_WATERMARK ;

	m_psh.pszbmWatermark = MAKEINTRESOURCE(101);
	m_psh.hbmHeader = NULL;

	m_psh.hInstance = AfxGetInstanceHandle();
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
/*
** CPropSheet message handlers
*/


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


void
CPropSheet::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
    CString s;

#ifdef EVALUATION_RELEASE
    s.LoadString(IDS_SDKTITLE);
#else
    if (theInstall.m_DBATools)
    s.Format(IDS_TITLE_DBA, theInstall.m_installationcode);
    else
    s.Format(IDS_TITLE, theInstall.m_installationcode);
#endif /* EVALUATION_RELEASE */
    SetWindowText(s);
}

void
CPropSheet::OnPaint() 
{
    if (IsIconic())
    {
	CPaintDC dc(this); /* device context for painting */
	
	SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
	
	// Center icon in client rectangle
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

HCURSOR
CPropSheet::OnQueryDragIcon() 
{
    return (HCURSOR) theIcon;
}

BOOL
CPropSheet::OnInitDialog() 
{
    SetIcon(theIcon, TRUE); 	// Set big icon
    SetIcon(theIcon, FALSE);	// Set small icon
    HideButtons();

    return CPropertySheet::OnInitDialog();
}

void
CPropSheet::HideButtons()
{
    SetRedraw(FALSE);
    CWnd *wnd = GetDlgItem(0x3023);
    if (wnd) 
		wnd->ShowWindow(SW_HIDE);
    wnd = GetDlgItem(0x3024);
    if (wnd) 
		wnd->ShowWindow(SW_HIDE);
    wnd = GetDlgItem(0x3025);
    if (wnd) 
		wnd->ShowWindow(SW_HIDE);
    wnd = GetDlgItem(IDCANCEL);
    if (wnd) 
		wnd->ShowWindow(SW_HIDE);
	
    SetRedraw(TRUE);
    InvalidateRect(NULL,FALSE);

    CMenu *menu = GetSystemMenu(FALSE);
	if (menu)
		menu->EnableMenuItem(1, MF_BYPOSITION | MF_DISABLED);

}
