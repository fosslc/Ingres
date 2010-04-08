/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : mainfrm.h, header file
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Main Frame 
**               See the CONCEPT.DOC for more detail
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created.
** 20-Apr-2000 (uk$so01)
**    SIR #101252
**    When user start the VCBF, and if IVM is already running
**    then request IVM the start VCBF
** 09-Feb-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**/

#if !defined (MAINFRAME_HEADER)
#define MAINFRAME_HEADER

class CVcbfView;

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)


	// Operations
	//
public:
	CVcbfView* m_pVcbfView;
	void CloseApplication (BOOL bNormal = TRUE);
	UINT GetHelpID();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

	BOOL m_bEndSession;
	// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnClose();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	//}}AFX_MSG
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg BOOL OnQueryEndSession();
	afx_msg LONG OnExternalRequestShutDown(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnMakeActive(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);

#endif
/////////////////////////////////////////////////////////////////////////////
