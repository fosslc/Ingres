/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vnodemgr.h, Header File
*
**    Project  : Virtual Node Manager.
**    Author   : UK Sotheavut
**
**    Purpose  : Main Application
**
**    HISTORY:
** 13-Oct-2003 (schph01)
**    SIR 109864 manage -vnode command line option for ingnet utility
*/
#if !defined(AFX_VNODEMGR_H__2D0C26C6_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_VNODEMGR_H__2D0C26C6_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"  // main symbols
#include "maindoc.h"
#include "parsearg.h"
class CaNodeDataAccess;

/////////////////////////////////////////////////////////////////////////////
// CappVnodeManager:
// See vnodemgr.cpp for the implementation of this class
//

class CappVnodeManager : public CWinApp
{
public:
	CappVnodeManager();

	void InitStrings();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappVnodeManager)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//
	// Implementation
	CString m_strNodeLocal;
	CdMainDoc* m_pDocument;
	CaNodeDataAccess* m_pNodeAccess;
	CString m_strMessage;

	CString m_strFolderNode;
	CString m_strFolderServer;
	CString m_strFolderLogin;
	CString m_strFolderConnection;
	CString m_strFolderAttribute;
	CString m_strFolderAdvancedNode;
	CString m_strUnavailable;
	CString m_strNoNodes;

	CString m_strInstallPath;
	CString m_strHelpFile;

	CaCommandLine m_cmdLine;
	//{{AFX_MSG(CappVnodeManager)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpOnlineSupport();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CappVnodeManager theApp;


BOOL APP_HelpInfo(UINT nContextHelpID = 0);


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VNODEMGR_H__2D0C26C6_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
