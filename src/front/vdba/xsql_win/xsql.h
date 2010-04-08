/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xsql.h: header file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : main header file for the XSQL application
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 11-May-2007 (karye01)
**    SIR #118282 added Help menu item for support url.
** 14-Aug-2008 (smeke01) b129684
**    Adding missing command-line help for groups & roles.
*/

#if !defined(AFX_XSQL_H__1D04F614_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
#define AFX_XSQL_H__1D04F614_EFC9_11D5_8795_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "sessimgr.h"
#include "cmdargs.h"

class CappSqlQuery : public CWinApp
{
public:
	CappSqlQuery();
	void OutOfMemoryMessage()
	{
		AfxMessageBox (m_tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
	}
	CaSessionManager& GetSessionManager(){return m_sessionManager;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappSqlQuery)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL


	// Implementation
protected:
	TCHAR m_tchszOutOfMemoryMessage[256];
	CaSessionManager m_sessionManager;

public:
	TCHAR m_tchszSqlProperty[32];
	TCHAR m_tchszSqlData[32];
	TCHAR m_tchszContainerData[32];
	CaArgumentLine m_cmdLineArg;
	//{{AFX_MSG(CappSqlQuery)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpOnlineSupport();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#if defined (EDBC)
#define IDS_APP_TITLE_XX                  IDS_EDBC_APP_TITLE
#define IDS_INVALID_CMDLINE_XX            IDS_EDBC_INVALID_CMDLINE
#define IDS_MSG_II_SYSTEM_NOT_DEFINED_XX  IDS_EDBC_MSG_II_SYSTEM_NOT_DEFINED
#else
#if defined (_OPTION_GROUPxROLE)
#define IDS_INVALID_CMDLINE_XX            IDS_INVALID_CMDLINE_WITH_GROUP_AND_ROLE
#else
#define IDS_INVALID_CMDLINE_XX            IDS_INVALID_CMDLINE
#endif
#define IDS_APP_TITLE_XX                  AFX_IDS_APP_TITLE
#define IDS_MSG_II_SYSTEM_NOT_DEFINED_XX  IDS_MSG_II_SYSTEM_NOT_DEFINED
#endif




extern CappSqlQuery theApp;
extern CappSqlQuery OnHelpOnlineSupport;
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XSQL_H__1D04F614_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
