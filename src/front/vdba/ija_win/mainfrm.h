/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mainfrm.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : MFC (Frame View Doc)
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 26-Jul-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management. Add menu Help\Help Topics that invokes the general help.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#if !defined(AFX_MAINFRM_H__DA2AAD9A_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_MAINFRM_H__DA2AAD9A_AF05_11D3_A322_00C04F1F754A__INCLUDED_
#include "usplittr.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CfMainFrame : public CFrameWnd
{
protected:
	CfMainFrame();
	DECLARE_DYNCREATE(CfMainFrame)

public:
	CView* GetRightView();
	CView* GetLeftView();
	CuUntrackableSplitterWnd& GetSplitterWnd(){return m_Splitterwnd;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
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
	CuUntrackableSplitterWnd m_Splitterwnd;
	BOOL m_bAllViewCreated;

	// Generated message map functions
protected:
	//{{AFX_MSG(CfMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnRefresh();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnDefaultHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__DA2AAD9A_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
