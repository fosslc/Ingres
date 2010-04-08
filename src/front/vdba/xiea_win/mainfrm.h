/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.h : interface of the CfMainFrame class
**    Project  : Ingres II / IEA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main frame
**
** History:
**
** 16-Oct-2001 (uk$so01)
**    Created
**
**/

#if !defined(AFX_MAINFRM_H__849C6E89_C211_11D5_8784_00C04F1F754A__INCLUDED_)
#define AFX_MAINFRM_H__849C6E89_C211_11D5_8784_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "view.h"

class CfMainFrame : public CFrameWnd
{
public:
	CfMainFrame();
protected: 
	DECLARE_DYNAMIC(CfMainFrame)


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CfMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CvChildView  m_wndView;

	// Generated message map functions
protected:
	//{{AFX_MSG(CfMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	//}}AFX_MSG
	afx_msg long OnExportAssistant(WPARAM w, LPARAM l);

	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__849C6E89_C211_11D5_8784_00C04F1F754A__INCLUDED_)
