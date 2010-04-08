/****************************************************************************************
//                                                                                     //        
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : VewVNode.h, Header file    (MDI View of CFrmVNodeMDIChild)            //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Virtual Node Dialog Bar Resizable becomes a MDI Child when            //
//               it is not a Docking View.                                             //
****************************************************************************************/
#ifndef VEWVNODE_HEADER
#define VEWVNODE_HEADER
#include "vtree2.h"
class CViewVNodeMDIChild : public CView
{
    CTreeCtrl* m_pTree;
public:
    CTreeCtrl* GetVNodeTreeCtrl () {return m_pTree;};
protected:
	CViewVNodeMDIChild();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CViewVNodeMDIChild)
	BOOL UpdateNodeDlgItemButton (UINT idMenu);
	CTreeItemNodes * GetSelectedVirtNodeItem();

    // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewVNodeMDIChild)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

protected:
	virtual ~CViewVNodeMDIChild();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CViewVNodeMDIChild)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnUpdateVnodebar01(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar02(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar03(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar04(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar05(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar06(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar07(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar08(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar09(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar10(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVnodebar11(CCmdUI* pCmdUI);
	afx_msg void OnVnodebar01();
	afx_msg void OnVnodebar02();
	afx_msg void OnVnodebar03();
	afx_msg void OnVnodebar04();
	afx_msg void OnVnodebar05();
	afx_msg void OnVnodebar06();
	afx_msg void OnVnodebar07();
	afx_msg void OnVnodebar08();
	afx_msg void OnVnodebar09();
	afx_msg void OnVnodebar10();
	afx_msg void OnVnodebar11();
	afx_msg void OnUpdateVnodebarscratch(CCmdUI* pCmdUI);
	afx_msg void OnVnodebarscratch();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif

