/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.h : interface of the CfSda class
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
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
** 17-Nov-2004 (uk$so01)
**    SIR #113475 (Add new functionality to allow user to save the results 
**    of a comparison into a CSV file.
** 11-May-2007 (karye01)
**    SIR #118282 added Help menu item for support url.
*/


#if !defined(AFX_MAINFRM_H__2F5E5359_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
#define AFX_MAINFRM_H__2F5E5359_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CvSda;
class CfSda : public CFrameWnd
{
protected: 
	CfSda();
	DECLARE_DYNCREATE(CfSda)

public:
	CvSda* CfSda::GetVsdaView();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfSda)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CfSda();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CToolBar    m_wndToolBar;

	// Generated message map functions
protected:
	//{{AFX_MSG(CfSda)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnAppAbout();
	afx_msg void OnHelpOnlineSupport();
	afx_msg void OnDiscard();
	afx_msg void OnUndiscard();
	afx_msg void OnUpdateDiscard(CCmdUI* pCmdUI);
	afx_msg void OnUpdateUndiscard(CCmdUI* pCmdUI);
	afx_msg void OnCompare();
	afx_msg void OnUpdateAccessVdba(CCmdUI* pCmdUI);
	afx_msg void OnAccessVdba();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnDefaultHelp();
	afx_msg void OnUpdateSaveDifferences(CCmdUI* pCmdUI);
	afx_msg void OnSaveDifferences();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__2F5E5359_CF0B_11D6_87E7_00C04F1F754A__INCLUDED_)
