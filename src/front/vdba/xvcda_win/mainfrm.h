/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.h : interface of the CfCda class
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
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
** 11-May-2007 (karye01)
**    SIR #118282 added Help menu item for support url.
**/


#if !defined(AFX_MAINFRM_H__EAF345D9_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_MAINFRM_H__EAF345D9_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CvCda;
class CfCda : public CFrameWnd
{
	
protected: // create from serialization only
	CfCda();
	DECLARE_DYNCREATE(CfCda)

public:
	CvCda* CfCda::GetVcdaView();


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfCda)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CfCda();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CfCda)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnAppAbout();
	afx_msg void OnHelpOnlineSupport();
	afx_msg void OnCompare();
	afx_msg void OnSaveSnapshot();
	afx_msg void OnRestoreInstallation();
	afx_msg void OnUpdateRestoreInstallation(CCmdUI* pCmdUI);
	afx_msg void OnDefaultHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__EAF345D9_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
