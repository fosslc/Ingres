/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : iia.h : main header file for the IIA application
**    Project  : Ingres II / IIA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Parse the arguments
**
** History:
**
** 07-Feb-2001 (uk$so01)
**    Created
** 18-Jul-2002 (hanje04)
**	Bug 108296
**	To stop IIA crashing on UNIX when loading libsvriia.so for the first 
**	time we make a dummy call to InitCommonControls() to force IIA to link
**	to libcomctl32.so. Declare it here.
**
**/

#if !defined(AFX_IIA_H__4E271B44_FCE9_11D4_873E_00C04F1F754A__INCLUDED_)
#define AFX_IIA_H__4E271B44_FCE9_11D4_873E_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"
#include "parsearg.h"

#ifdef MAINWIN
void InitCommonControls(void);
#endif


class CappImportAssistant : public CWinApp
{
public:
	CappImportAssistant();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappImportAssistant)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//
	// Implementation
public:
	CaCommandLine m_cmdLine;
	//{{AFX_MSG(CappImportAssistant)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
extern CappImportAssistant theApp;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IIA_H__4E271B44_FCE9_11D4_873E_00C04F1F754A__INCLUDED_)
