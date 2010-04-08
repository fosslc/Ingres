/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsda.h : main header file for the VSDA application
**    Project  : INGRES II/ Visual Schema Diff Control (vdda.exe).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main application vdda.exe (container of vsda.ocx)
**
** History:
**
** 23-Sep-2002 (uk$so01)
**    Created
*/

#if !defined(AFX_VSDA_H__2F5E5355_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
#define AFX_VSDA_H__2F5E5355_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"
#include "sessimgr.h"


class CappVsda : public CWinApp
{
public:
	CappVsda();
	void OutOfMemoryMessage()
	{
		AfxMessageBox (m_tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
	}
	CaSessionManager& GetSessionManager(){return m_sessionManager;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappVsda)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL
protected:
	TCHAR m_tchszOutOfMemoryMessage[256];
	CaSessionManager m_sessionManager;

public:
	// Implementation
	//{{AFX_MSG(CappVsda)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
extern CappVsda theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSDA_H__2F5E5355_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
