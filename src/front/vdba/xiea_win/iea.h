/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iea.h : main header file for the IEA application
**    Project  : Ingres II / IEA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main application
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
**
**/

#if !defined(AFX_IEA_H__849C6E85_C211_11D5_8784_00C04F1F754A__INCLUDED_)
#define AFX_IEA_H__849C6E85_C211_11D5_8784_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"
#include "parsearg.h"


class CappExportAssistant : public CWinApp
{
public:
	CappExportAssistant();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappExportAssistant)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	// Implementation

public:
	CaCommandLine m_cmdLine;
	//{{AFX_MSG(CappExportAssistant)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


extern CappExportAssistant theApp;
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IEA_H__849C6E85_C211_11D5_8784_00C04F1F754A__INCLUDED_)
