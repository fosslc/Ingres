/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ija.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main App IJA (Ingres Journal Analyser)
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 28-Sep-2001 (hanje04)
**    BUG 105941
**    To stop SEGV on UNIX when loading ActiveX object (ijactrl.ocx) at 
**    startup. InitCommonControls must be called during InitInstance.
**    Declare it here.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 11-May-2007 (karye01)
**    SIR #118282, added Help menu item for support url.
**/

#if !defined(AFX_IJA_H__DA2AAD96_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_IJA_H__DA2AAD96_AF05_11D3_A322_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"       // main symbols
#include "parsearg.h"       // parse arguments

#ifdef MAINWIN
void InitCommonControls(void);
#endif

class CappIja : public CWinApp
{
public:
	CappIja();
	CaSessionManager& GetSessionManager(){return m_sessionManager;}

	CString m_strAllUser;
	CString m_strMsgErrorDBStar;
	CaCommandLine m_cmdLine;
	CaSessionManager m_sessionManager;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappIja)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	// Implementation
	//{{AFX_MSG(CappIja)
	afx_msg void OnHelpOnlineSupport();
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CappIja theApp;
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);

#define ID_HELP_GENERAL 1000
#define ID_HELP_REFRESH 1001


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IJA_H__DA2AAD96_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
