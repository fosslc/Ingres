/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : mainview.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Virtual Node Manager.                                                 //
//    Author   : UK Sotheavut, Detail implementation.                                  //
//                                                                                     //
//    Purpose  : Main View (Frame, View, Doc design). Tree View of Node Data           //
//    HISTORY:
//    uk$so01:  (24-jan-2000, Sir #100102)
//               Add the "Performance Monitor" command in toolbar and menu
//    schph01:  (28-Nov-2000, SIR 102945)
//               Grayed menu "Add Installation Password..." when the selection
//               on tree is on another branch that "Nodes" and "(Local)".
//    schph01:  (10-Oct-2003, SIR #109864)
//               Add menu "Nodes Defined on Selected Node"
****************************************************************************************/
#if !defined(AFX_MAINVIEW_H__2D0C26CE_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_MAINVIEW_H__2D0C26CE_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CvMainView : public CTreeView
{
protected: // create from serialization only
	CvMainView();
	DECLARE_DYNCREATE(CvMainView)

public:
	CdMainDoc* GetDocument();
	void InitializeData();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvMainView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CvMainView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	BOOL m_bInitialize;
	CImageList m_ImageList;
	
	// Generated message map functions
protected:
	//{{AFX_MSG(CvMainView)
	afx_msg void OnNodeAdd();
	afx_msg void OnNodeAlter();
	afx_msg void OnNodeDrop();
	afx_msg void OnNodeRefresh();
	afx_msg void OnNodeSqltest();
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnUpdateNodeAdd(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNodeAlter(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNodeDrop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNodeSqltest(CCmdUI* pCmdUI);
	afx_msg void OnNodeAddInstallation();
	afx_msg void OnNodeTest();
	afx_msg void OnUpdateNodeTest(CCmdUI* pCmdUI);
	afx_msg void OnNodeDom();
	afx_msg void OnUpdateNodeDom(CCmdUI* pCmdUI);
	afx_msg void OnNodeMonitor();
	afx_msg void OnUpdateNodeMonitor(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNodeAddInstallation(CCmdUI* pCmdUI);
	afx_msg void OnNodeEditView();
	afx_msg void OnUpdateNodeEditView(CCmdUI* pCmdUI);
	afx_msg void OnManageLocalvnode();
	afx_msg void OnUpdateManageLocalvnode(CCmdUI* pCmdUI);
	afx_msg void OnPopupNodeEditView();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in mainview.cpp
inline CdMainDoc* CvMainView::GetDocument()
   { return (CdMainDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINVIEW_H__2D0C26CE_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
