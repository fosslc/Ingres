/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xipm.h  : Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main header file for the XIPM application
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 11-May-2007 (karye01)
**    SIR #118282 added Help menu item for support url.
*/


#if !defined(AFX_XIPM_H__AE712EF4_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
#define AFX_XIPM_H__AE712EF4_E8C7_11D5_8792_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"  // main symbols
#include "cmdargs.h"
#include "fileerr.h"	// Added by ClassView

class CappIpm : public CWinApp
{
public:
	CappIpm();
	void OutOfMemoryMessage()
	{
		AfxMessageBox (m_tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
	}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CappIpm)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	// Implementation
	TCHAR m_tchszIpmProperty[32];
	TCHAR m_tchszIpmData[32];
	TCHAR m_tchszContainerData[32];
	CaArgumentLine m_cmdLineArg;
protected:
	TCHAR m_tchszOutOfMemoryMessage[256];

public:
	//{{AFX_MSG(CappIpm)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpOnlineSupport();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CappIpm theApp;
extern CappIpm OnHelpOnlineSupport;
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XIPM_H__AE712EF4_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
