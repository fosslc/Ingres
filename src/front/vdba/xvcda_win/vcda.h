/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcda.h : main header file for the VCDA application
**    Project  : VCDA (Container)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main application InitInstance
**
** History:
**
** 01-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
**/

#if !defined(AFX_VCDA_H__EAF345D5_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_VCDA_H__EAF345D5_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"
#include "cmdargs.h"

class CappVCDA : public CWinApp
{
public:
	CappVCDA();
	CaArgumentLine m_cmdArg;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappVCDA)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	// Implementation
	//{{AFX_MSG(CappVCDA)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CappVCDA theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VCDA_H__EAF345D5_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
