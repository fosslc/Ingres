/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: PropSheet.cpp : Property sheet for the High Availability Option setup wizard
** 
** Description:
** 	Add and delete the property pages of the setup wizard.
**
** History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
** 	03-aug-2004 (wonst02)
** 		Add header comments.
**	30-nov-2004 (wonst02) Bug 113345
**	    Remove HELP from the property sheet.
*/

#include "stdafx.h"
#include "wincluster.h"
#include "installcode.h"
#include "clusterpage.h"
#include "dependencies.h"
#include "final.h"
#include "PropSheet.h"
#include ".\propsheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WcPropSheet

IMPLEMENT_DYNAMIC(WcPropSheet, CPropertySheet)

WcPropSheet::WcPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
	, m_GlobalError(0)
{
	m_psh.dwFlags &= ~(PSH_HASHELP);
	AddPage(new WcWelcome());
	AddPage(new WcInstallCode());
	AddPage(new WcClusterPage());
	AddPage(new WcDependencies());
	AddPage(new WcFinal());
	SetWizardMode();
}


WcPropSheet::~WcPropSheet()
{
	for (int i=0; i<GetPageCount(); i++)
		delete GetPage(i);
}


BEGIN_MESSAGE_MAP(WcPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(WcPropSheet)
	ON_WM_ACTIVATE()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// WcPropSheet message handlers

BOOL WcPropSheet::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch(wParam)
	{
	case IDCANCEL:
		if (!AskUserYN(IDS_AREYOUSURE))
			return TRUE;
	}

	return CPropertySheet::OnCommand(wParam, lParam);
}

BOOL WcPropSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	SetIcon(theIcon, TRUE);
	SetIcon(theIcon, FALSE);
	return bResult;
}

void WcPropSheet::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CPropertySheet::OnActivate(nState, pWndOther, bMinimized);

	CString s;
	s.LoadString(IDS_TITLE);
	SetWindowText(s);	
}

void WcPropSheet::OnClose() 
{
	if(AskUserYN(IDS_AREYOUSURE))
		CPropertySheet::OnClose();
}

void WcPropSheet::OnPaint() 
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

HCURSOR WcPropSheet::OnQueryDragIcon() 
{
	return (HCURSOR) theIcon;
}

/*
** 	Display message in a pop-up message box.
*/
UINT WcPropSheet::MyMessageBox(UINT uiStrID,UINT uiFlags,LPCSTR param)
{
	UINT ret;
	CString title;
	CString text;
	
	title.LoadString(IDS_TITLE);
	if (param)
		text.Format(uiStrID,param);
	else
		text.LoadString(uiStrID);

	HWND hwnd=property ? property->m_hWnd : 0;
	if(!hwnd)
	 uiFlags|=MB_APPLMODAL;

	ret=::MessageBox(hwnd, text,title,uiFlags);
	return ret;
}

// Display a system error message in a message box
void WcPropSheet::SysError(UINT stringId,	// A string identifier from the local resource file
						   LPCSTR insertChr,// A string to be inserted into the message text
						   DWORD lastError) // A system error code to be looked up and inserted
{
	CString	Message, Title;
	CHAR	outmsg[512];
	Message.LoadString(stringId);
	Title.LoadString(IDS_TITLE);
	LPVOID lpMsgBuf = NULL;
	if (lastError)
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL );
	if (insertChr && lastError)
		sprintf(outmsg, Message, insertChr, (LPCTSTR)lpMsgBuf);
	else if (insertChr)
		sprintf(outmsg, Message, insertChr, "");
	else if (lastError)
		sprintf(outmsg, Message, (LPCTSTR)lpMsgBuf, "");
	else
		sprintf(outmsg, Message, "", "");
	MessageBox(outmsg, Title, MB_ICONEXCLAMATION | MB_OK);
	// Free the buffer.
	if (lpMsgBuf)
		LocalFree( lpMsgBuf );
	m_GlobalError = lastError ? lastError : stringId;
}
