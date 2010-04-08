/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.cpp : implementation of the CfSda class
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame/View/Doc Architecture (frame)
**
** History:
**
** 23-Sep-2002 (uk$so01)
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
** 17-Nov-2004 (uk$so01)
**    SIR #113475 (Add new functionality to allow user to save the results 
**    of a comparison into a CSV file.
** 19-Nov-2004 (noifr01)
**    (sir 113475) fixed problem at the end of the file, resulting from a
**    propagation problem from another codeline.
** 19-Jan-2005 (komve01)
**    BUG #113768 / ISSUE 13913697: 
**	  GUI tools display incorrect year in the Copyright Information.
** 21-Jan-2005 (drivi01)
**    Removed bad symbols ">>" probably left over from integration.
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
*/


#include "stdafx.h"
#include <htmlhelp.h>
#include "vsda.h"
#include "mainfrm.h"
#include "vsdadoc.h"
#include "vsdaview.h"
#include "tlsfunct.h"
#include "copyright.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CfSda

IMPLEMENT_DYNCREATE(CfSda, CFrameWnd)

BEGIN_MESSAGE_MAP(CfSda, CFrameWnd)
	//{{AFX_MSG_MAP(CfSda)
	ON_WM_CREATE()
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_SUPPORTONLINE, OnHelpOnlineSupport)
	ON_COMMAND(ID_DISCARD, OnDiscard)
	ON_COMMAND(ID_UNDISCARD, OnUndiscard)
	ON_UPDATE_COMMAND_UI(ID_DISCARD, OnUpdateDiscard)
	ON_UPDATE_COMMAND_UI(ID_UNDISCARD, OnUpdateUndiscard)
	ON_COMMAND(ID_COMPARE, OnCompare)
	ON_UPDATE_COMMAND_UI(ID_ACCESS_VDBA, OnUpdateAccessVdba)
	ON_COMMAND(ID_ACCESS_VDBA, OnAccessVdba)
	ON_WM_HELPINFO()
	ON_COMMAND(ID_DEFAULT_HELP, OnDefaultHelp)
	ON_UPDATE_COMMAND_UI(ID_SAVE_DIFFERENCES, OnUpdateSaveDifferences)
	ON_COMMAND(ID_SAVE_DIFFERENCES, OnSaveDifferences)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfSda construction/destruction

CfSda::CfSda()
{
	// TODO: add member initialization code here
	
}

CfSda::~CfSda()
{
}

int CfSda::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	CMenu* pMenu = GetMenu();
	if (pMenu)
	{
		pMenu->DeleteMenu(1, MF_BYPOSITION); // Edit
	}
	/*
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	*/
	return 0;
}

BOOL CfSda::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CfSda diagnostics

#ifdef _DEBUG
void CfSda::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CfSda::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CfSda message handlers


// Menu command to open support site in default browser
void CfSda::OnHelpOnlineSupport()
{
	ShellExecute(0, _T("open"), "http://www.ingres.com/support.php", 0, 0, SW_SHOWNORMAL);
}


void CfSda::OnAppAbout() 
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

CvSda* CfSda::GetVsdaView()
{
	//
	// The view might not actuve, do not use get active view !
	CdSda* pDoc = (CdSda*)GetActiveDocument();
	if (pDoc)
	{
		POSITION pos = pDoc->GetFirstViewPosition();
		if (pos)
		{
			CvSda* pView = (CvSda*)pDoc->GetNextView(pos);
			return pView;
		}
	}
	return NULL;
}

void CfSda::OnDiscard() 
{
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			pVsdaCtrl->DiscardItem();
	}
}

void CfSda::OnUndiscard() 
{
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			pVsdaCtrl->UndiscardItem();
	}
}

void CfSda::OnUpdateDiscard(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			bEnable = pVsdaCtrl->IsEnableDiscard();
	}
	pCmdUI->Enable(bEnable);
}

void CfSda::OnUpdateUndiscard(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			bEnable = pVsdaCtrl->IsEnableUndiscard();
	}
	pCmdUI->Enable(bEnable);
}

void CfSda::OnCompare() 
{
	CvSda* pView = GetVsdaView();
	if (pView && pView->m_pDlgMain)
	{
		(pView->m_pDlgMain)->DoCompare();
	}
}

void CfSda::OnUpdateAccessVdba(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			bEnable = pVsdaCtrl->IsEnableAccessVdba();
	}
	pCmdUI->Enable(bEnable);
}

void CfSda::OnAccessVdba() 
{
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			pVsdaCtrl->AccessVdba();
	}
}

BOOL CfSda::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		if (pView->m_pDlgMain->m_bCompared)
		{
			CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
			pVsdaCtrl->SendMessageToDescendants(WM_HELP);
			return TRUE;
		}
	}
	APP_HelpInfo(m_hWnd, 0);
	return CFrameWnd::OnHelpInfo(pHelpInfo);
}

void CfSda::OnDefaultHelp() 
{
	APP_HelpInfo(m_hWnd, 0);
}

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#if defined (MAINWIN)
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += _T("vdda.chm");
	}

	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}

void CfSda::OnUpdateSaveDifferences(CCmdUI* pCmdUI) 
{
	BOOL bEnable = FALSE;
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			bEnable = pVsdaCtrl->IsResultFrameVisible();
	}
	pCmdUI->Enable(bEnable);
}

void CfSda::OnSaveDifferences() 
{
	CvSda* pView = GetVsdaView();
	if (pView)
	{
		CuVddaControl* pVsdaCtrl = pView->GetVsdaControl();
		if (pVsdaCtrl)
			pVsdaCtrl->DoWriteFile();
	}
}
