/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.cpp : implementation of the CfCda class
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : frame/view/doc architecture (frame)
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 30-Jul-2004 (uk$so01)
**    SIR #112708, Change the AboutBox due to UI Standard.
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 14-Feb-2006 (drivi01)
**    Update the year to 2006.
** 08-Jan-2007 (drivi01)
**    Update the year to 2007.
** 11-May-2007 (karye01)
**    SIR #118282 added Help menu item for support url.
** 07-Jan-2008 (drivi01)
**    Created copyright.h header for Visual DBA suite.
**    Redefine copyright year as constant defined in new
**    header file.  This will ensure single point of update
**    for variable year.
** 20-Mar-2009 (smeke01) b121832
**    Product year is returned by INGRESII_BuildVersionInfo() so does 
**    not need to be set in here.
**/

#include "stdafx.h"
#include <htmlhelp.h>
#include "vcda.h"
#include "mainfrm.h"
#include "vcdaview.h"
#include "vcdadoc.h"
#include "tlsfunct.h"
#include "copyright.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CfCda

IMPLEMENT_DYNCREATE(CfCda, CFrameWnd)

BEGIN_MESSAGE_MAP(CfCda, CFrameWnd)
	//{{AFX_MSG_MAP(CfCda)
	ON_WM_CREATE()
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_SUPPORTONLINE, OnHelpOnlineSupport)
	ON_COMMAND(ID_COMPARE, OnCompare)
	ON_COMMAND(ID_SAVE_SNAPSHOT, OnSaveSnapshot)
	ON_COMMAND(ID_RESTORE, OnRestoreInstallation)
	ON_UPDATE_COMMAND_UI(ID_RESTORE, OnUpdateRestoreInstallation)
	ON_COMMAND(ID_DEFAULT_HELP, OnDefaultHelp)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfCda construction/destruction

CfCda::CfCda()
{
	// TODO: add member initialization code here
	
}

CfCda::~CfCda()
{
}

int CfCda::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	CMenu* pMenu = GetMenu();
	if (pMenu)
	{
		pMenu->DeleteMenu(1, MF_BYPOSITION); // Edit
#if defined (_NO_MENU_COMPARE)
		CMenu* pFile = pMenu->GetSubMenu(0);
		if (pFile)
		{
			pFile->DeleteMenu(0, MF_BYPOSITION); // Compare
			pFile->DeleteMenu(0, MF_BYPOSITION); // Separator
		}
#endif
	}
	return 0;
}

BOOL CfCda::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CfCda diagnostics

#ifdef _DEBUG
void CfCda::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CfCda::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CfCda message handlers


// Menu command to open support site in default browser
void CfCda::OnHelpOnlineSupport()
{
       ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
}


void CfCda::OnAppAbout() 
{
	BOOL bOK = TRUE;
	CString strDllName = _T("tksplash.dll");
#if defined (MAINWIN)
	#if defined (hpb_us5)
	strDllName  = _T("libtksplash.sl");
	#else
	strDllName  = _T("libtksplash.so");
	#endif
#endif
	HINSTANCE hLib = LoadLibrary(strDllName);
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		bOK = FALSE;
	if (bOK)
	{
		void (PASCAL *lpDllEntryPoint)(LPCTSTR, LPCTSTR, short, UINT);
		(FARPROC&)lpDllEntryPoint = GetProcAddress (hLib, "AboutBox");
		if (lpDllEntryPoint == NULL)
			bOK = FALSE;
		else
		{
			CString strTitle;
			CString strAbout;
			int year;
			CString strVer;
			// 0x00000002 : Show Copyright
			// 0x00000004 : Show End-User License Aggreement
			// 0x00000008 : Show the WARNING law
			// 0x00000010 : Show the Third Party Notices Button
			// 0x00000020 : Show System Info Button
			// 0x00000040 : Show Tech Support Button
			UINT nMask = 0x00000002|0x00000008;
			INGRESII_BuildVersionInfo (strVer, year);

			strAbout.Format (IDS_PRODUCT_VERSION, strVer);
			strTitle.LoadString (AFX_IDS_APP_TITLE);
			(*lpDllEntryPoint)(strTitle, strAbout, (short)year, nMask);
		}
		FreeLibrary(hLib);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_FAIL_2_LOCATEDLL, (LPCTSTR)strDllName);
		AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
	}
}

CvCda* CfCda::GetVcdaView()
{
	//
	// The view might not actuve, do not use get active view !
	CdCda* pDoc = (CdCda*)GetActiveDocument();
	if (pDoc)
	{
		POSITION pos = pDoc->GetFirstViewPosition();
		if (pos)
		{
			CvCda* pView = (CvCda*)pDoc->GetNextView(pos);
			return pView;
		}
	}
	return NULL;
}

void CfCda::OnCompare() 
{
	CvCda* pView = GetVcdaView();
	if (pView && pView->m_pDlgMain)
	{
		(pView->m_pDlgMain)->DoCompare();
	}
}

void CfCda::OnSaveSnapshot() 
{
	CvCda* pView = GetVcdaView();
	if (pView && pView->m_pDlgMain)
	{
		(pView->m_pDlgMain)->DoSaveInstallation();
	}
}

void CfCda::OnRestoreInstallation() 
{
	CvCda* pView = GetVcdaView();
	if (pView && pView->m_pDlgMain)
	{
		(pView->m_pDlgMain->m_cVcda).RestoreInstallation();
	}
	
}

void CfCda::OnUpdateRestoreInstallation(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CfCda::OnDefaultHelp() 
{
	APP_HelpInfo(m_hWnd, 0);
}

BOOL CfCda::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CvCda* pView = GetVcdaView();
	if (pView && pView->m_pDlgMain)
	{
		if (pView->m_pDlgMain->m_bCompared)
		{
			pView->m_pDlgMain->m_cVcda.SendMessageToDescendants(WM_HELP);
			return TRUE;
		}
	}
	APP_HelpInfo(m_hWnd, 0);
	return CFrameWnd::OnHelpInfo(pHelpInfo);
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#ifdef MAINWIN
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += _T("vcda.chm");
	}

	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}
