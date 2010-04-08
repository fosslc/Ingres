/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : mainfrm.h, Header File                                                //
//                                                                                     //
//                                                                                     //
//    Project  : Virtual Node Manager.                                                 //
//    Author   : UK Sotheavut, Detail implementation.                                  //
//                                                                                     //
//    Purpose  : Main Frame (Frame, View, Doc design).                                 //
****************************************************************************************/
#if !defined(AFX_MAINFRM_H__2D0C26CA_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_MAINFRM_H__2D0C26CA_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CfMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CfMainFrame();
	DECLARE_DYNCREATE(CfMainFrame)

public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CfMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

	// Generated message map functions
protected:
	//{{AFX_MSG(CfMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnHelpTopic();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__2D0C26CA_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
